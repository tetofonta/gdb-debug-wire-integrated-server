//
// Created by stefano on 03/08/22.
//
#include <dw/debug_wire.h>
#include "panic.h"

#define BEGIN_PRESERVE_REG(reg)             uint16_t __save##reg = dw_cmd_get(reg); do{
#define END_PRESERVE_REG(reg)               dw_cmd_set(reg, &__save##reg); }while(0);


static dw_sw_brkpt_t * _dw_is_swprkpt(void){
    for (int i = 0; i < debug_wire_g.swbrkpt_n; ++i)
        if(debug_wire_g.swbrkpt[i].address == debug_wire_g.program_counter) return &debug_wire_g.swbrkpt[i];
    return NULL;
}

void debug_wire_halt(void){
    dw_cmd_halt(); //status -> halted
}
void debug_wire_resume(uint8_t context){
    if(!debug_wire_g.halted) panic();

    dw_sw_brkpt_t * swbrkpt = _dw_is_swprkpt();
    if(swbrkpt != NULL){
        dw_cmd_set(DW_CMD_REG_PC, &debug_wire_g.program_counter);
        dw_set_context(DW_GO_CNTX_SWBP ^ (debug_wire_g.run_timers << 5));
        dw_cmd_set(DW_CMD_REG_IR, &swbrkpt->instruction);
        dw_cmd_go(1);
        return;
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
    dw_cmd_send_multiple_consts(( 0xD0 | DW_CMD_REG_PC ), 2, 0, 0);
    dw_set_context(DW_GO_CNTX_CONTINUE);
    dw_cmd_send_multiple_consts(( 0xD0 | DW_CMD_REG_PC ), 2, 0, 0);
    dw_cmd_go(0);
}

void _debug_wire_read_sram(uint16_t from, uint16_t to, void * read_buffer){
    BEGIN_PRESERVE_REG(DW_CMD_REG_PC)
    BEGIN_PRESERVE_REG(DW_CMD_REG_HWBP)

            uint16_t from_be = from << 8 | from >> 8;
            uint16_t to_be = to << 8 | to >> 8;

            dw_set_context(DW_GO_CNTX_MEM);
            dw_cmd_set(DW_CMD_REG_PC, &from_be);
            dw_cmd_set(DW_CMD_REG_HWBP, &to_be);
            dw_cmd_mode_set(DW_MODE_REGS_READ);

            od_uart_clear();
            dw_cmd_start_sram_cycle();
            od_uart_recv(read_buffer, to - from);

    END_PRESERVE_REG(DW_CMD_REG_HWBP)
    END_PRESERVE_REG(DW_CMD_REG_PC)
}
