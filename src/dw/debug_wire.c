//
// Created by stefano on 03/08/22.
//
#include <dw/debug_wire.h>
#include "panic.h"

void debug_wire_halt(void){
    dw_cmd_halt(); //status -> halted
}

static dw_sw_brkpt_t * _dw_is_swprkpt(void){
    for (int i = 0; i < debug_wire_g.swbrkpt_n; ++i)
        if(debug_wire_g.swbrkpt[i].address == debug_wire_g.program_counter) return &debug_wire_g.swbrkpt[i];
    return NULL;
}

void debug_wire_resume(uint8_t context){
    if(!debug_wire_g.halted) panic();

    dw_sw_brkpt_t * swbrkpt = _dw_is_swprkpt();
    if(swbrkpt != NULL){
        dw_cmd_set(DW_CMD_REG_PC, &debug_wire_g.program_counter);
        dw_set_context(DW_GO_CNTX_SWBP ^ (debug_wire_g.run_timers << 5));
        dw_cmd_set(DW_CMD_REG_IR, &swbrkpt->instruction);
        dw_cmd_go(1);
    }

    dw_cmd_set(DW_CMD_REG_PC, &debug_wire_g.program_counter);
    dw_set_context(context ^ (debug_wire_g.run_timers << 5));
    dw_cmd_set(DW_CMD_REG_PC, &debug_wire_g.program_counter); //WHY?
    dw_cmd_go(0);
}

void debug_wire_device_reset(void){
    if(!debug_wire_g.halted)
        dw_cmd_halt();

    dw_cmd_reset();
    dw_cmd_multi_const(( 0x0D | DW_CMD_REG_PC ), 2, 0, 0);
    dw_set_context(DW_GO_CNTX_CONTINUE);
    dw_cmd_multi_const(( 0x0D | DW_CMD_REG_PC ), 2, 0, 0);
    dw_cmd_go(0);
}