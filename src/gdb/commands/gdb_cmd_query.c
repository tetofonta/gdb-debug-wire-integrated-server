//
// Created by stefano on 08/08/22.
//
#include <gdb/gdb.h>
#include <gdb/utils.h>
#include <dw/debug_wire.h>
#include <avr/pgmspace.h>
#include "usb/usb_cdc.h"
#include "leds.h"

void gdb_cmd_query(char *buffer, uint16_t len) {
    if (!memcmp_P(buffer, PSTR("Supported"), 9)) {
        gdb_send_PSTR(PSTR("$PacketSize=3c;swbreak+#1b"), 26);

        if(gdb_state_g.state ==  GDB_STATE_SIGHUP){
            return;
        }

        //gdb is connected!
        GDB_LED_ON();
        gdb_state_g.state = GDB_STATE_IDLE;
        debug_wire_device_reset();
        gdb_state_g.state = GDB_STATE_SIGTRAP;
    } else if (buffer[1] == 'C') gdb_send_PSTR(PSTR("$QC01#f5"), 8);
    else if (!memcmp_P(buffer, PSTR("fThr"), 4)) gdb_send_PSTR(PSTR("$m01#ce"), 7); //qfThreadinfo
    else if (!memcmp_P(buffer, PSTR("sThr"), 4)) gdb_send_PSTR(PSTR("$l#6c"), 5); //qsThreadInfo
    else if (!memcmp_P(buffer, PSTR("Rcmd,"), 5)){
        //monitor command
        if (!memcmp_P(buffer + 5, PSTR("72"), 2)){ //R - eset
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
            debug_wire_device_reset();
        }
    }
    else gdb_send_empty();
}