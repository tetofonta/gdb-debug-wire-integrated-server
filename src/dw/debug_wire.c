//
// Created by stefano on 03/08/22.
//
#include <dw/debug_wire.h>
#include "panic.h"

#define BEGIN_PRESERVE_REG(reg)             uint16_t __save##reg = dw_cmd_get(reg); do{
#define END_PRESERVE_REG(reg)               dw_cmd_set(reg, &__save##reg); }while(0);
#define BEGIN_PRESERVE_GP_REG(reg)             uint8_t __save##reg = _dw_read_register(reg); do{
#define END_PRESERVE_GP_REG(reg)               _dw_write_register(reg, __save##reg); }while(0);

#define REG_Z_L     30
#define REG_Z_H     31

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
static uint8_t _dw_read_register(uint8_t reg){
    _debug_wire_reg_rw_setup(reg, reg + 1, DW_MODE_REGS_READ);
    return od_uart_recv_byte();
}
static void _dw_write_register(uint8_t reg, uint8_t data){
    _debug_wire_reg_rw_setup(reg, reg + 1, DW_MODE_REGS_WRITE);
    od_uart_tx_byte(data);
}
static void _dw_read_registers(uint8_t from, uint8_t to, void * buffer){
    _debug_wire_reg_rw_setup(from, to, DW_MODE_REGS_READ);
    od_uart_recv(buffer, to - from);
}
static void _dw_write_registers(uint8_t from, uint8_t to, void * buffer){
    _debug_wire_reg_rw_setup(from, to, DW_MODE_REGS_WRITE);
    od_uart_send(buffer, to - from);
}


static void _debug_wire_mem_r_setup(uint16_t from, uint16_t len, uint8_t target){
    _dw_write_register(REG_Z_L, from & 0xFF);
    _dw_write_register(REG_Z_H, from >> 8);

    dw_cmd_set_multi(DW_CMD_REG_PC, 0, 0);
    dw_cmd_set_multi(DW_CMD_REG_HWBP, (len * 2) >> 8, (len * 2) & 0xFF);
    dw_cmd_mode_set(target);
    dw_cmd_start_mem_cycle();
}
static void _dw_read_sram(uint8_t from, uint8_t len, void * buffer){
    _debug_wire_mem_r_setup(from, len, DW_MODE_SRAM_READ);
    od_uart_recv(buffer, len);
}

void debug_wire_read_registers(void * read_buffer, uint16_t len, uint16_t address) {
    if (!debug_wire_g.halted) return;
    BEGIN_PRESERVE_REG(DW_CMD_REG_PC)
    BEGIN_PRESERVE_REG(DW_CMD_REG_HWBP)

            _dw_read_registers(address, address + len, read_buffer);

    END_PRESERVE_REG(DW_CMD_REG_HWBP)
    END_PRESERVE_REG(DW_CMD_REG_PC)
}
void debug_wire_write_registers(void *buffer, uint16_t len, uint16_t address) {
    if (!debug_wire_g.halted) return;
    BEGIN_PRESERVE_REG(DW_CMD_REG_PC)
        BEGIN_PRESERVE_REG(DW_CMD_REG_HWBP)

            _dw_write_registers(address, address + len, buffer);

                END_PRESERVE_REG(DW_CMD_REG_HWBP)
            END_PRESERVE_REG(DW_CMD_REG_PC)
}

void debug_wire_read_sram(void *buffer, uint16_t len, uint16_t address) {
    if (!debug_wire_g.halted) return;
    BEGIN_PRESERVE_REG(DW_CMD_REG_PC)
        BEGIN_PRESERVE_REG(DW_CMD_REG_HWBP)
            BEGIN_PRESERVE_GP_REG(REG_Z_L)
                BEGIN_PRESERVE_GP_REG(REG_Z_H)

                    _dw_read_sram(address, len, buffer);

                END_PRESERVE_GP_REG(REG_Z_H)
            END_PRESERVE_GP_REG(REG_Z_L)
        END_PRESERVE_REG(DW_CMD_REG_HWBP)
    END_PRESERVE_REG(DW_CMD_REG_PC)
}

