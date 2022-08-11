//
// Created by stefano on 03/08/22.
//
#include <dw/debug_wire_ll.h>
#include <dw/debug_wire.h>
#include "panic.h"
#include "avr_isa.h"

static dw_sw_brkpt_t *_dw_is_swprkpt(void) {
    for (uint8_t i = 0; i < debug_wire_g.swbrkpt_n; ++i)
        if (debug_wire_g.swbrkpt[i].address == debug_wire_g.program_counter) return &debug_wire_g.swbrkpt[i];
    return NULL;
}

void debug_wire_resume(uint8_t context) {
    if (!debug_wire_g.halted) return;

    dw_sw_brkpt_t *swbrkpt = _dw_is_swprkpt();
    if (swbrkpt != NULL) {
        dw_cmd_set(DW_CMD_REG_PC, &debug_wire_g.program_counter);
        dw_set_context(DW_GO_CNTX_SWBP ^ (debug_wire_g.run_timers << 5));
        dw_cmd_set(DW_CMD_REG_IR, &swbrkpt->opcode);
        if(context == DW_GO_CNTX_SS)
            dw_cmd_ss(1);
        else
            dw_cmd_go(1);
        return;
    }

    dw_cmd_set(DW_CMD_REG_PC, &debug_wire_g.program_counter);
    dw_set_context(context ^ (debug_wire_g.run_timers << 5));
    dw_cmd_set(DW_CMD_REG_PC, &debug_wire_g.program_counter); //WHY?
    if(context == DW_GO_CNTX_SS)
        dw_cmd_ss(0);
    else
        dw_cmd_go(0);
}

void debug_wire_device_reset(void) {
    if (!debug_wire_g.halted)
        dw_cmd_halt();
    dw_cmd_reset();
    debug_wire_g.program_counter = 0;
    dw_cmd_set_multi(DW_CMD_REG_HWBP, 0xff, 0xff);
}

uint8_t debug_wire_halt(void){
    if (!debug_wire_g.halted)
        return dw_cmd_halt();
    return 1;
}

struct dw_state cur_state;
void dw_env_open(uint8_t env_type){
    if(env_type == 0) {
        env_type++;
        goto save_ir;
    }

    cur_state.pc = dw_cmd_get(DW_CMD_REG_PC);
    cur_state.hwbp = dw_cmd_get(DW_CMD_REG_HWBP);
    if(env_type == 1) return;
    env_type--;

    dw_ll_registers_read(r30, r31, &cur_state.z);
    if(env_type == 1) return;
    env_type--;

    dw_ll_registers_read(r26, r27, &cur_state.x);
    dw_ll_registers_read(r28, r29, &cur_state.y);
    save_ir: cur_state.ir = dw_cmd_get(DW_CMD_REG_IR);
    if(env_type == 1) return;
    env_type--;

    cur_state.reg0 = dw_ll_register_read(r0);
    if(env_type == 1) return;
    env_type--;

    cur_state.reg1 = dw_ll_register_read(r1);
}

void dw_env_close(uint8_t env_type){
    if(env_type == 0){
        env_type = 1;
        goto pop_ir;
    }
    if(env_type == 5){
        dw_ll_register_write(r1, cur_state.reg1);
        env_type--;
    }
    if(env_type == 4){
        dw_ll_register_write(r0, cur_state.reg0);
        env_type--;
    }
    if(env_type == 3){
        dw_ll_registers_write(r26, r27, &cur_state.x);
        dw_ll_registers_write(r28, r29, &cur_state.y);
        pop_ir:
        dw_cmd_set(DW_CMD_REG_IR, &cur_state.ir);
        env_type--;
    }
    if(env_type == 2){
        dw_ll_registers_write(r30, r31, &cur_state.z);
        env_type--;
    }
    if(env_type == 1){
        dw_cmd_set(DW_CMD_REG_PC, &cur_state.pc);
        dw_cmd_set(DW_CMD_REG_HWBP, &cur_state.hwbp);
        env_type--;
    }
}