//
// Created by stefano on 08/08/22.
//
#include <gdb/gdb.h>
#include <gdb/utils.h>
#include <dw/debug_wire.h>
#include <avr/pgmspace.h>
#include "usb/usb_cdc.h"
#include "leds.h"
#include "dw/debug_wire_ll.h"
#include "gdb/rtt.h"
#include "panic.h"

void gdb_cmd_query(char *buffer, uint16_t len) {
    if (!memcmp_P(buffer, PSTR("Supported"), 9)) {
        gdb_send_PSTR(PSTR("$PacketSize=3c;swbreak+#1b"), 26);

        if(gdb_state_g.state ==  GDB_STATE_SIGHUP){
            return;
        }
        gdb_rtt_enable = 0;
        rtt_set_state(0);

        //gdb is connected!
        GDB_LED_ON();
        gdb_state_g.state = GDB_STATE_IDLE;
        debug_wire_device_reset();
        gdb_state_g.state = GDB_STATE_SIGTRAP;
    }
    else if (buffer[1] == 'C') gdb_send_PSTR(PSTR("$QC01#f5"), 8);
    else if (!memcmp_P(buffer, PSTR("fThr"), 4)) gdb_send_PSTR(PSTR("$m01#ce"), 7); //qfThreadinfo
    else if (!memcmp_P(buffer, PSTR("sThr"), 4)) gdb_send_PSTR(PSTR("$l#6c"), 5); //qsThreadInfo
    else if (!memcmp_P(buffer, PSTR("Rcmd,"), 5)){
        //monitor command
        if (!memcmp_P(buffer + 5, PSTR("7265"), 4)){
            debug_wire_device_reset();
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
        } //re - set
        else if (!memcmp_P(buffer + 5, PSTR("7274"), 4)) {
            dw_env_open(DW_ENV_SRAM_RW);
            if(!memcmp_P(buffer + 13, PSTR("64"), 2)){ //rtt ]d - isable
                rtt_set_state(0);
            } else {
                rtt_set_state(2);
            }
            dw_env_close(DW_ENV_SRAM_RW);
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
        } //rt - t (d - isable/enable)
        else if (!memcmp_P(buffer + 5, PSTR("64"), 2)){
            MUST_SUCCEED(debug_wire_halt(), 1);
            dw_cmd(DW_CMD_DISABLE);
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
        } // d - isable
        else if (!memcmp_P(buffer + 5, PSTR("74"), 2)){
            if(!memcmp_P(buffer + 5 + 14, PSTR("64"), 2)) debug_wire_g.run_timers = 0;
            else debug_wire_g.run_timers = 1;
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
        } //t - imers (d - isable/ enable)
        else if (!memcmp_P(buffer + 5, PSTR("73"), 2)){
            uint32_t signature = byte2hex(debug_wire_g.device.signature & 0xFF) | (uint32_t) byte2hex(debug_wire_g.device.signature >> 8) << 16;
            gdb_message((const char *) &signature, PSTR("O"), 4, 1);
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
        } //s - ignature (d - isable/ enable)
        else
            gdb_send_empty();

    }
    else gdb_send_empty();
}