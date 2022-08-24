//
// Created by stefano on 08/08/22.
//

#include <usb/usb_cdc.h>
#include "gdb/gdb.h"
#include "gdb/commands.h"
#include "dw/debug_wire.h"
#include "dw/debug_wire_ll.h"
#include "panic.h"
#include "gdb/utils.h"
#include "leds.h"
#include "avr_isa.h"
#include "gdb/rtt.h"
#include "gdb/pstr.h"

struct gdb_state gdb_state_g;
static uint8_t ack_enabled = 1;
static uint8_t is_cmd_running = 0;
static uint8_t halt_happened = 0;
static uint16_t last_used_freq;

void gdb_init(uint16_t freq) {
    if(freq == 0) freq = last_used_freq;
    _delay_ms(100);

    if (dw_init((uint32_t) freq * 1000)) {
        debug_wire_device_reset();
        rtt_set_state(0);
        debug_wire_resume(DW_GO_CNTX_CONTINUE);
        gdb_state_g.state = GDB_STATE_DISCONNECTED;
        last_used_freq = freq;
        return;
    }

    gdb_state_g.state = GDB_STATE_SIGHUP;
}

void gdb_deinit(void){
    dw_ll_deinit();
    gdb_state_g.state = GDB_STATE_DISCONNECTED;
}

static void gdb_send_state(void) {
    cdc_buffer.as_byte_buffer[0] = 'S';
    cdc_buffer.as_byte_buffer[1] = nib2hex((gdb_state_g.state >> 4) & 0x0F);
    cdc_buffer.as_byte_buffer[2] = nib2hex(gdb_state_g.state & 0x0F);
    gdb_send((const char *) cdc_buffer.as_byte_buffer, 3);
}

void gdb_send_PSTR(const char *data, uint16_t len) {
    usb_cdc_write_PSTR(data, len);
}

void gdb_send(const char *data, uint16_t len) {
    gdb_send_begin();
    uint8_t checksum = gdb_send_add_data(data, len, 0);
    gdb_send_finalize(checksum);
}

void gdb_send_ack(void) {
    if (ack_enabled) {
        GDB_LED_ON();
        usb_cdc_write_PSTR(GDB_PKT_ANSW_ACK, 1);
    }
}

void gdb_send_nack(void) {
    usb_cdc_write_PSTR(GDB_PKT_ANSW_NACK, 1);
}

void gdb_send_empty(void) {
    usb_cdc_write_PSTR(GDB_PKT_ANSW_EMPTY, 4);
}

uint8_t gdb_send_begin(void) {
    usb_cdc_write_PSTR(GDB_PKT_ANSW_EMPTY, 1);
    return 0;
}

uint8_t gdb_send_add_data(const char *data, uint16_t len, uint8_t checksum) {
    usb_cdc_write(data, len);
    while (len--) checksum += *(data++);
    return checksum;
}

uint8_t gdb_send_add_data_PSTR(const char *data, uint16_t len, uint8_t checksum) {
    usb_cdc_write_PSTR(data, len);
    while (len--) checksum += pgm_read_byte(data++);
    return checksum;
}

void gdb_send_finalize(uint8_t checksum) {
    uint16_t chksm = byte2hex(checksum & 0xFF);
    usb_cdc_write_PSTR(GDB_PKT_ANSW_EMPTY + 1, 1);
    usb_cdc_write(&chksm, 2);
}

static void gdb_parse_command(const char *buf, uint16_t len) {
    if (is_flash_write_in_progress && *buf != 'X' && *buf != 'M') {
        finalize_flash_write();
    }

    switch (*buf) {
        case '?':
            gdb_send_state();
            break;
        case 'C':
        case 'c':
            dw_env_open(DW_ENV_FLASH_WRITE);
            dw_ll_flush_breakpoints(cdc_buffer.as_word_buffer, USB_CDC_BUFFER_WORDS);
            dw_env_close(DW_ENV_FLASH_WRITE);

            gdb_state_g.state = GDB_STATE_IDLE;
            gdb_state_g.last_context = DW_GO_CNTX_HWBP;
            debug_wire_resume(DW_GO_CNTX_HWBP);
            break;
        case 'S':
        case 's':
            dw_env_open(DW_ENV_FLASH_WRITE);
            dw_ll_flush_breakpoints(cdc_buffer.as_word_buffer, USB_CDC_BUFFER_WORDS);
            dw_env_close(DW_ENV_FLASH_WRITE);

            gdb_state_g.state = GDB_STATE_IDLE;
            gdb_state_g.last_context = DW_GO_CNTX_SS;
            debug_wire_resume(DW_GO_CNTX_SS);
            break;
        case 'q':
            gdb_cmd_query((char *) (cdc_buffer.as_byte_buffer + 1), len - 1);
            break;
        case 'v':
            gdb_cmd_v((char *) (cdc_buffer.as_byte_buffer + 1), len - 1);
            break;
        case 'G':
            gdb_cmd_write_registers((char *) cdc_buffer.as_byte_buffer, len);
            break;
        case 'g':
            gdb_cmd_read_registers();
            break;
        case 'm':
            gdb_cmd_read_memory((char *) cdc_buffer.as_byte_buffer + 1, len - 1);
            break;
        case 'z':
        case 'Z':
            gdb_cmd_breakpoint((char *) cdc_buffer.as_byte_buffer, len);
            break;
        case 'k':
        case 'D':
            gdb_cmd_end(true, cdc_buffer.as_word_buffer, USB_CDC_BUFFER_WORDS);
        case 'H':
        case 'T':
            gdb_send_PSTR(GDB_REP_OK, 6);
            break;
        case 'r':
        case 'R':
            gdb_cmd_end(false, cdc_buffer.as_word_buffer, USB_CDC_BUFFER_WORDS);
            gdb_send_state();
            break;
        case 'M':
            gdb_cmd_write_memory((char *) cdc_buffer.as_byte_buffer, len);
            break;
        case '!':
        default:
            gdb_send_empty();
    }
}

static void gdb_handle_command(void) {
    uint8_t checksum = 0;
    uint8_t expected_hex = 0;
    GDB_LED_OFF();
    uint16_t len = usb_cdc_read(cdc_buffer.as_byte_buffer, USB_CDC_BUFFER_SIZE);
    uint16_t newlen = len;
    if (cdc_buffer.as_byte_buffer[len - 3] == '#') newlen -= 3;
    while (newlen--)
        checksum += *(cdc_buffer.as_byte_buffer + newlen);

    if (cdc_buffer.as_byte_buffer[len - 3] == '#') {
        is_cmd_running = 0;
        expected_hex = hex2nib(cdc_buffer.as_byte_buffer[len - 2]) << 4 | hex2nib(cdc_buffer.as_byte_buffer[len - 1]);
        if (expected_hex != checksum) usb_cdc_write_PSTR(GDB_PKT_ANSW_NACK, 1);
        else {
            gdb_send_ack();
            gdb_parse_command((char *) cdc_buffer.as_byte_buffer, len);
        }
        return;
    }

    //todo check checksum for incoming flushed data...
    gdb_send_ack();
    gdb_parse_command((char *) cdc_buffer.as_byte_buffer, len);
    Endpoint_SelectEndpoint(CDC_RX_EPADDR);
}

void gdb_message(const char * buf, const char * init, uint8_t len, uint8_t len_init){
    uint8_t checksum = gdb_send_begin();
    uint16_t data;
    checksum = gdb_send_add_data_PSTR(init, len_init, checksum);
    while(len--){
        data = byte2hex(*buf++);
        checksum = gdb_send_add_data((const char *) &data, 2, checksum);
    }
    checksum = gdb_send_add_data_PSTR(GDB_PKT_ANSW_LF, 2, checksum);
    gdb_send_finalize(checksum);
}

void gdb_task(void) {

    if(halt_happened){
        if (gdb_state_g.state != GDB_STATE_IDLE) {
            halt_happened = 0;
            return;
        }
        dw_env_open(DW_ENV_FLASH_READ);
        dw_ll_flash_read(BE(debug_wire_g.program_counter) * 2, 2, &debug_wire_g.last_opcode);
        dw_env_close(DW_ENV_FLASH_READ);

        if(gdb_rtt_enable){
            dw_env_open(DW_ENV_SRAM_RW);
            uint8_t len = rtt_get_last_message(cdc_buffer.as_byte_buffer);
            dw_env_close(DW_ENV_SRAM_RW);

            if(len > 0) {
                gdb_message((const char *) cdc_buffer.as_byte_buffer, GDB_PKT_ANSW_RTT, len, 11);
                if(gdb_state_g.last_context == DW_GO_CNTX_CONTINUE || gdb_state_g.last_context == DW_GO_CNTX_HWBP) {
                    debug_wire_resume(gdb_state_g.last_context);
                    halt_happened = 0;
                    return;
                }
            }
        }

        gdb_state_g.state = illegal_opcode(BE(debug_wire_g.last_opcode)) ? GDB_STATE_SIGILL : GDB_STATE_SIGTRAP;
        gdb_send_state();
        halt_happened = 0;
        return;
    }

    Endpoint_SelectEndpoint(CDC_RX_EPADDR);
    if (Endpoint_IsOUTReceived()) {
        uint8_t cmd_type;
        usb_cdc_read(&cmd_type, 1);
        if(is_cmd_running){
            if(cmd_type == '#') is_cmd_running = 0;
        } else {
            switch (cmd_type) {
                case '+':
                    break; //ack, just ignore it
                case '-':
                    if (!is_cmd_running) panic(); //nack, should resend but too ram heavy for now. maybe I'll think out something...
                    break;
                case 0x03:
                    gdb_state_g.state = GDB_STATE_SIGINT;
                    gdb_state_g.state = debug_wire_halt() ? GDB_STATE_SIGINT : GDB_STATE_SIGHUP;
                    gdb_send_state();
                    break;
                case '$':
                    is_cmd_running = 1;
                    gdb_handle_command();
                    Endpoint_SelectEndpoint(CDC_RX_EPADDR);
                    if(!Endpoint_IsReadWriteAllowed()){
                        Endpoint_ClearOUT();
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void on_dw_mcu_halt(void) {
    halt_happened = 1;
}