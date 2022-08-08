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
uint8_t buffer[64];
uint8_t aux;

void gdb_init(void){
    _delay_ms(100);
    if(dw_init(2*8000000)){//todo get frequency from eeprom
        debug_wire_device_reset();
        debug_wire_resume(DW_GO_CNTX_CONTINUE);
        gdb_state_g.state = GDB_STATE_IDLE;
        return;
    }

    gdb_state_g.state = GDB_STATE_SIGHUP;
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
    usb_cdc_write_PSTR(PSTR("+"), 1);
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

void gdb_send_finalize(uint8_t checksum){
    uint16_t chksm = byte2hex(checksum & 0xFF);
    usb_cdc_write_PSTR(PSTR("#"), 1);
    usb_cdc_write(&chksm, 2);
}

static void gdb_parse_command(const char * buf, uint16_t len){
    switch (*buf) {
        case '?':
            buffer[0] = 'S';
            buffer[1] = nib2hex((gdb_state_g.state >> 4) & 0x0F);
            buffer[2] = nib2hex(gdb_state_g.state & 0x0F);
            gdb_send(buffer, 3);
            break;
        case 'q':
            gdb_cmd_query((char *) (buffer + 1), len);
            break;
        case 'v':
            gdb_cmd_v((char *) (buffer + 1), len);
            break;
        case 'g':
            gdb_cmd_read_registers();
            break;
        case 'k':
        case 'D':
            gdb_cmd_end();
        case 'H':
        case 'T':
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
            break;
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
        expected_hex = hex2nib(buffer[len - 2]) << 4 | hex2nib(buffer[len - 1]);
        if(expected_hex != checksum) usb_cdc_write_PSTR(PSTR("-"), 1);
        else gdb_parse_command(buffer, len);
        return;
    }

    //todo check checksum for incoming flushed data...
    gdb_parse_command(buffer, len);
    while(len != 0){
        len = usb_cdc_read(buffer, 64);
    }
}

void gdb_task(void){
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    Endpoint_SelectEndpoint(CDC_RX_EPADDR);
    if(Endpoint_IsOUTReceived()){
        uint8_t cmd_type = usb_cdc_read_byte();
        switch (cmd_type) {
            case '+':
                break; //ack, just ignore it
            case '-':
                panic(); //nack, should resend but too ram heavy for now. maybe I'll think out something...
            case 0x03: break;
            case '$':
                gdb_handle_command();
                break;
            default:
                break;
        }
    }
}
