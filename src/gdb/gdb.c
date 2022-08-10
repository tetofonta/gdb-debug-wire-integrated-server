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

struct gdb_state gdb_state_g;
uint8_t ack_enabled = 1;
uint8_t is_cmd_running = 0;
uint8_t buffer[64];

void gdb_init(void){
    _delay_ms(100);

    if(dw_init(2*8000000)){//todo get frequency from eeprom
        debug_wire_device_reset();
        debug_wire_resume(DW_GO_CNTX_CONTINUE);
        gdb_state_g.state = GDB_STATE_DISCONNECTED;
        return;
    }

    gdb_state_g.state = GDB_STATE_SIGHUP;
    panic();
}

static void gdb_send_state(void){
    buffer[0] = 'S';
    buffer[1] = nib2hex((gdb_state_g.state >> 4) & 0x0F);
    buffer[2] = nib2hex(gdb_state_g.state & 0x0F);
    gdb_send(buffer, 3);
}

void gdb_send_PSTR(const char * data, uint16_t len){
    gdb_send_ack();
    usb_cdc_write_PSTR(data, len);
}

void gdb_send(const char * data, uint16_t len){
    gdb_send_begin();
    uint8_t checksum = gdb_send_add_data(data, len, 0);
    gdb_send_finalize(checksum);
}

void gdb_send_ack(void){
    if(ack_enabled) usb_cdc_write_PSTR(PSTR("+"), 1);
}

void gdb_send_nack(void){
    usb_cdc_write_PSTR(PSTR("-"), 1);
}

void gdb_send_empty(void){
    gdb_send_ack();
    usb_cdc_write_PSTR(PSTR("$#00"), 4);
}

uint8_t gdb_send_begin(void){
    gdb_send_ack();
    usb_cdc_write_PSTR(PSTR("$"), 1);
    return 0;
}

uint8_t gdb_send_add_data(const char * data, uint16_t len, uint8_t checksum){
    usb_cdc_write(data, len);
    while(len--) checksum += *(data++);
    return checksum;
}

uint8_t gdb_send_add_data_PSTR(const char * data, uint16_t len, uint8_t checksum){
    usb_cdc_write_PSTR(data, len);
    while(len--) checksum += pgm_read_byte(data++);
    return checksum;
}


void gdb_send_finalize(uint8_t checksum){
    uint16_t chksm = byte2hex(checksum & 0xFF);
    usb_cdc_write_PSTR(PSTR("#"), 1);
    usb_cdc_write(&chksm, 2);
}

static void gdb_parse_command(const char * buf, uint16_t len){
    switch (*buf) {
        case '?':
            gdb_send_state();
            break;
        case 'C':
        case 'c':
            dw_env_open(DW_GO_CNTX_FLASH_WRT);
            dw_ll_flush_breakpoints((uint16_t *) buffer, 32);
            dw_env_close(DW_GO_CNTX_FLASH_WRT);

            debug_wire_resume(DW_GO_CNTX_HWBP);
            gdb_state_g.state = GDB_STATE_IDLE;
            gdb_send_ack();
            break;
        case 'S':
        case 's':
            dw_env_open(DW_GO_CNTX_FLASH_WRT);
            dw_ll_flush_breakpoints((uint16_t *) buffer, 32);
            dw_env_close(DW_GO_CNTX_FLASH_WRT);

            gdb_send_ack();
            gdb_state_g.state = GDB_STATE_IDLE;
            debug_wire_resume(DW_GO_CNTX_SS);
            break;
        case 'q':
            gdb_cmd_query((char *) (buffer + 1), len - 1 );
            break;
        case 'v':
            gdb_cmd_v((char *) (buffer + 1), len - 1);
            break;
        case 'G':
            gdb_cmd_write_registers(buffer, len);
            break;
        case 'g':
            gdb_cmd_read_registers();
            break;
        case 'm':
            gdb_cmd_read_memory(buffer + 1, len - 1);
            break;
        case 'z':
        case 'Z':
            gdb_cmd_breakpoint(buffer, len);
            break;
        case 'k':
        case 'D':
            gdb_cmd_end(true, (uint16_t *) buffer, 32);
        case 'H':
        case 'T':
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
            break;
        case 'r':
        case 'R':
            gdb_cmd_end(false, (uint16_t *) buffer, 32);
            gdb_send_ack();
            gdb_send_state();
            break;
        case 'M':
        case 'X':

        case '!':
        default:
            gdb_send_empty();
    }
}

static void gdb_handle_command(void){
    uint8_t checksum = 0;
    uint8_t expected_hex = 0;

    uint16_t len = usb_cdc_read(buffer, 64);
    uint16_t newlen = len;
    if(buffer[len - 3] == '#') newlen -= 3;
    while(newlen--)
        checksum += *(buffer + newlen);

    if(buffer[len - 3] == '#') {
        is_cmd_running = 0;
        expected_hex = hex2nib(buffer[len - 2]) << 4 | hex2nib(buffer[len - 1]);
        if(expected_hex != checksum) usb_cdc_write_PSTR(PSTR("-"), 1);
        else gdb_parse_command(buffer, len);
        return;
    }

    //todo check checksum for incoming flushed data...
    gdb_parse_command(buffer, len);
}

void gdb_task(void){
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    Endpoint_SelectEndpoint(CDC_RX_EPADDR);
    if(Endpoint_IsOUTReceived()){
        uint8_t cmd_type;
        usb_cdc_read(&cmd_type, 1);
        switch (cmd_type) {
            case '+':
                break; //ack, just ignore it
            case '-':
                if(!is_cmd_running) panic(); //nack, should resend but too ram heavy for now. maybe I'll think out something...
                break;
            case 0x03:
                gdb_state_g.state = GDB_STATE_SIGINT;
                gdb_state_g.state = debug_wire_halt() ? GDB_STATE_SIGINT : GDB_STATE_SIGHUP;
                gdb_send_state();
                break;
            case '#':
                is_cmd_running = 0;
                break;
            case '$':
                is_cmd_running = 1;
                gdb_handle_command();
                Endpoint_SelectEndpoint(CDC_RX_EPADDR);
                Endpoint_ClearOUT();
                break;
            default:
                break;
        }
    }
}

inline void on_dw_mcu_halt(void){
    if(gdb_state_g.state != GDB_STATE_IDLE) return;
    gdb_state_g.state = GDB_STATE_SIGTRAP;

    uint8_t tmp = ack_enabled;
    ack_enabled = 0;
    gdb_send_state();
    ack_enabled = tmp;
}
