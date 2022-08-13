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

struct gdb_state gdb_state_g;
uint8_t ack_enabled = 1;
uint8_t is_cmd_running = 0;

void gdb_init(void) {
    _delay_ms(100);

    if (dw_init(2 * 8000000)) {//todo get frequency from eeprom
        debug_wire_device_reset();
        debug_wire_resume(DW_GO_CNTX_CONTINUE, false);
        gdb_state_g.state = GDB_STATE_DISCONNECTED;
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
        usb_cdc_write_PSTR(PSTR("+"), 1);
    }
}

void gdb_send_nack(void) {
    usb_cdc_write_PSTR(PSTR("-"), 1);
}

void gdb_send_empty(void) {
    usb_cdc_write_PSTR(PSTR("$#00"), 4);
}

uint8_t gdb_send_begin(void) {
    usb_cdc_write_PSTR(PSTR("$"), 1);
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
    usb_cdc_write_PSTR(PSTR("#"), 1);
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
            dw_env_open(DW_GO_CNTX_FLASH_WRT);
            dw_ll_flush_breakpoints(cdc_buffer.as_word_buffer, USB_CDC_BUFFER_WORDS);
            dw_env_close(DW_GO_CNTX_FLASH_WRT);

            debug_wire_resume(DW_GO_CNTX_HWBP, false);
            gdb_state_g.state = GDB_STATE_IDLE;
            break;
        case 'S':
        case 's':
            dw_env_open(DW_GO_CNTX_FLASH_WRT);
            dw_ll_flush_breakpoints(cdc_buffer.as_word_buffer, USB_CDC_BUFFER_WORDS);
            dw_env_close(DW_GO_CNTX_FLASH_WRT);

            gdb_state_g.state = GDB_STATE_IDLE;
            debug_wire_resume(DW_GO_CNTX_SS, true);
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
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
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
        if (expected_hex != checksum) usb_cdc_write_PSTR(PSTR("-"), 1);
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

void gdb_task(void) {
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
                    if(!Endpoint_IsReadWriteAllowed()){
                        Endpoint_SelectEndpoint(CDC_RX_EPADDR);
                        Endpoint_ClearOUT();
                    }
                    break;
                default:
                    break;
            }
        }
//        Endpoint_ClearOUT();
//        switch (cmd_type) {
//            case '+':
//                dw_ll_flash_read(0, 64, cdc_buffer.as_byte_buffer);
//                usb_cdc_write(cdc_buffer.as_byte_buffer, 64);
//                dw_ll_flash_read(64, 64, cdc_buffer.as_byte_buffer);
//                usb_cdc_write(cdc_buffer.as_byte_buffer, 64);
//                dw_ll_flash_read(128, 64, cdc_buffer.as_byte_buffer);
//                usb_cdc_write(cdc_buffer.as_byte_buffer, 64);
//                dw_ll_flash_read(192, 64, cdc_buffer.as_byte_buffer);
//                usb_cdc_write(cdc_buffer.as_byte_buffer, 64);
//                break;
//            case '-':
//                memcpy_P(cdc_buffer.as_byte_buffer, PSTR("Z0,80,2#30"), 10);
//                gdb_cmd_breakpoint(cdc_buffer.as_byte_buffer, 10);
//                usb_cdc_write(debug_wire_g.swbrkpt, debug_wire_g.swbrkpt_n * sizeof(dw_sw_brkpt_t));
//                break;
//            case '*':
//                dw_ll_flush_breakpoints(cdc_buffer.as_word_buffer, 32);
//                break;
//        }
    }
}

inline void on_dw_mcu_halt(void) {
    if (gdb_state_g.state != GDB_STATE_IDLE) return;

    gdb_state_g.state = GDB_STATE_SIGTRAP;
    uint8_t tmp = ack_enabled;
    ack_enabled = 0;
    gdb_send_state();
    ack_enabled = tmp;
}