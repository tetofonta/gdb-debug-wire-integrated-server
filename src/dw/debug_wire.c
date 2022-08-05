//
// Created by stefano on 03/08/22.
//
#include <dw/debug_wire.h>
#include "panic.h"

static dw_sw_brkpt_t *_dw_is_swprkpt(void) {
    for (int i = 0; i < debug_wire_g.swbrkpt_n; ++i)
        if (debug_wire_g.swbrkpt[i].address == debug_wire_g.program_counter) return &debug_wire_g.swbrkpt[i];
    return NULL;
}
void debug_wire_halt(void) {
    dw_cmd_halt(); //status -> halted
}
void debug_wire_resume(uint8_t context) {
    if (!debug_wire_g.halted) return;

    dw_sw_brkpt_t *swbrkpt = _dw_is_swprkpt();
    if (swbrkpt != NULL) {
        dw_cmd_set(DW_CMD_REG_PC, &debug_wire_g.program_counter);
        dw_set_context(DW_GO_CNTX_SWBP ^ (debug_wire_g.run_timers << 5));
        dw_cmd_set(DW_CMD_REG_IR, &swbrkpt->instruction);
        dw_cmd_go(1);
        debug_wire_g.halted = 0;
        return;
    }

    dw_cmd_set(DW_CMD_REG_PC, &debug_wire_g.program_counter);
    dw_set_context(context ^ (debug_wire_g.run_timers << 5));
    dw_cmd_set(DW_CMD_REG_PC, &debug_wire_g.program_counter); //WHY?
    debug_wire_g.halted = 0;
    dw_cmd_go(0);
}
void debug_wire_device_reset(void) {
    if (!debug_wire_g.halted)
        dw_cmd_halt();

    dw_cmd_reset();
    dw_cmd_send_multiple_consts((0xD0 | DW_CMD_REG_PC), 2, 0, 0);
    dw_set_context(DW_GO_CNTX_CONTINUE);
    dw_cmd_send_multiple_consts((0xD0 | DW_CMD_REG_PC), 2, 0, 0);
    dw_cmd_go(0);
}

static void _debug_wire_reg_rw_setup(uint8_t from, uint8_t to, uint8_t target){
    uint16_t from_be = from << 8 | from >> 8;
    uint16_t to_be = to << 8 | to >> 8;

    dw_set_context(DW_GO_CNTX_MEM);
    dw_cmd_set(DW_CMD_REG_PC, &from_be);
    dw_cmd_set(DW_CMD_REG_HWBP, &to_be);
    dw_cmd_mode_set(target);
    if(target == DW_MODE_REGS_WRITE && (to - from) == 1)
        dw_cmd_start_mem_cycle_ss();
    else
        dw_cmd_start_mem_cycle();
}
uint8_t debug_wire_read_register(uint8_t reg){
    _debug_wire_reg_rw_setup(reg, reg + 1, DW_MODE_REGS_READ);
    return od_uart_recv_byte();
}
void debug_wire_write_register(uint8_t reg, uint8_t data){
    _debug_wire_reg_rw_setup(reg, reg + 1, DW_MODE_REGS_WRITE);
    od_uart_tx_byte(data);
}
void debug_wire_read_registers(uint8_t from, uint8_t to, void * buffer){
    _debug_wire_reg_rw_setup(from, to, DW_MODE_REGS_READ);
    od_uart_recv(buffer, to - from);
}
void debug_wire_write_registers(uint8_t from, uint8_t to, void * buffer){
    _debug_wire_reg_rw_setup(from, to, DW_MODE_REGS_WRITE);
    od_uart_send(buffer, to - from);
}

static void _debug_wire_mem_rw_setup(uint16_t from, uint16_t len, uint8_t target){
    debug_wire_write_registers(30, 32, &from);
    len *= 2;
    if(target == DW_MODE_SRAM_WRITE)
        len += 1;
    dw_cmd_set_multi(DW_CMD_REG_PC, 0, len & 1);
    dw_cmd_set_multi(DW_CMD_REG_HWBP, len >> 8, len & 0xFF);
    dw_cmd_mode_set(target);
    dw_cmd_start_mem_cycle();
}
void debug_wire_read_sram(uint8_t from, uint8_t len, void * buffer){
    _debug_wire_mem_rw_setup(from, len, DW_MODE_SRAM_READ);
    od_uart_recv(buffer, len);
}
void debug_wire_write_sram(uint8_t from, uint8_t len, void * buffer){
    _debug_wire_mem_rw_setup(from, len, DW_MODE_SRAM_WRITE);
    od_uart_send(buffer, len);
}
void debug_wire_read_flash(uint8_t from, uint8_t len, void * buffer){
    _debug_wire_mem_rw_setup(from, len, DW_MODE_FLASH_READ);
    od_uart_recv(buffer, len);
}

