//
// Created by stefano on 08/08/22.
//
#include <gdb/gdb.h>
#include <avr/pgmspace.h>
#include <dw/debug_wire.h>
#include "gdb/commands.h"

void gdb_cmd_v(char * buffer, uint16_t len){
    if(!memcmp_P(buffer, PSTR("Kill"), 4)){
        gdb_cmd_end(1);
    }
    gdb_send_empty();
}