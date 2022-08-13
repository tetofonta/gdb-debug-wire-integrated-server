//
// Created by stefano on 08/08/22.
//
#include <gdb/gdb.h>
#include <avr/pgmspace.h>
#include <dw/debug_wire_ll.h>
#include <stdbool.h>
#include "leds.h"

void gdb_cmd_end(uint8_t restart, uint16_t * buffer, uint16_t len){

    dw_ll_clear_breakpoints();
    dw_env_open(DW_GO_CNTX_FLASH_WRT);
    dw_ll_flush_breakpoints((uint16_t *) buffer, len);
    dw_env_close(DW_GO_CNTX_FLASH_WRT);

    debug_wire_device_reset();
    gdb_state_g.state = GDB_STATE_DISCONNECTED;

    if(restart) {
        debug_wire_resume(DW_GO_CNTX_CONTINUE, false);
        gdb_state_g.state = GDB_STATE_DISCONNECTED;
    }

    GDB_LED_OFF();
}