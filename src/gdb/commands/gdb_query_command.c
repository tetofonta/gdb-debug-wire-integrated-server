//
// Created by stefano on 08/08/22.
//
#include <gdb/gdb.h>
#include <dw/debug_wire.h>
#include <avr/pgmspace.h>

void gdb_cmd_query(char *buffer, uint16_t len) {
    if (!memcmp_P(buffer, PSTR("Supported"), 9)) {
        //gdb is connected!
        debug_wire_device_reset();
        gdb_state_g.state = GDB_STATE_SIGTRAP;
        gdb_send_PSTR(PSTR("$PacketSize=60;swbreak+;hwbreak+#35"), 35);

    } else if (buffer[1] == 'C') gdb_send_PSTR(PSTR("$QC01#f5"), 8);
    else if (!memcmp_P(buffer, PSTR("fThr"), 4)) gdb_send_PSTR(PSTR("$m01#ce"), 7); //qfThreadinfo
    else if (!memcmp_P(buffer, PSTR("sThr"), 4)) gdb_send_PSTR(PSTR("$l#6c"), 5); //qsThreadInfo
    else gdb_send_empty();
}