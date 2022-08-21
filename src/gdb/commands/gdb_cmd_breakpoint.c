//
// Created by stefano on 10/08/22.
//
#include <gdb/gdb.h>
#include <gdb/utils.h>
#include <gdb/pstr.h>
#include <avr/pgmspace.h>
#include <dw/debug_wire_ll.h>
#include "avr_isa.h"

void gdb_cmd_breakpoint(char * buffer, uint16_t len){

    if(!debug_wire_g.halted) return gdb_send_PSTR(GDB_ERR_05, 7);

    char action = *buffer++; //Z or z -> set or remove
    char type = *buffer++; //0 -> swbp, 1 -> hwbp, 2, 3, 4 -> unsupported whatchpoints
    buffer++; // ,
    len -= 3;
    uint64_t address = 0;

    parse_hex_until(',', buffer, &len, &address);
    address &= 0xFFFF;

    if(type == '0'){//swbp
        if(action == 'Z') {
            if (dw_ll_add_breakpoint(address >> 1)) gdb_send_PSTR(GDB_REP_OK, 6);
            else gdb_send_PSTR(GDB_ERR_03, 7);
        } else {
            dw_ll_remove_breakpoint(address >> 1);
            gdb_send_PSTR(GDB_REP_OK, 6);
        }
    } else if (type == '1'){ //hwbp
        if(action == 'Z'){
            address = BE(address);
            dw_cmd_set(DW_CMD_REG_HWBP, &address);
            gdb_send_PSTR(GDB_REP_OK, 6);
        } else {
            dw_cmd_set_multi(DW_CMD_REG_HWBP, 255, 255);
            gdb_send_PSTR(GDB_REP_OK, 6);
        }
    } else {
        gdb_send_empty();
    }
}