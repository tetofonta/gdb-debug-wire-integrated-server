//
// Created by stefano on 08/08/22.
//
#include <gdb/gdb.h>
#include <avr/pgmspace.h>
#include <dw/debug_wire.h>

void gdb_cmd_end(void){

    //todo update breakpoints

    debug_wire_device_reset();
    debug_wire_resume(DW_GO_CNTX_CONTINUE);
    gdb_state_g.state = GDB_STATE_IDLE;
}