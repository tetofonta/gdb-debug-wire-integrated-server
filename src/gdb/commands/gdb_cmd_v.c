//
// Created by stefano on 08/08/22.
//
#include <gdb/gdb.h>
#include <avr/pgmspace.h>
#include <dw/debug_wire.h>
#include "gdb/commands.h"
#include "gdb/pstr.h"

void gdb_cmd_v(char * buffer, uint16_t len){
    if(!memcmp_P(buffer, GDB_PKT_FILTER_MONITOR_VKILL, 4)){
        gdb_cmd_end(1,(uint16_t *) buffer, 32);
        gdb_send_PSTR(GDB_REP_OK, 6);
    }
    gdb_send_empty();
}