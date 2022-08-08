//
// Created by stefano on 07/08/22.
//

#ifndef ARDWINO_DEBUG_WIRE_LL_H
#define ARDWINO_DEBUG_WIRE_LL_H
#include <stdint.h>
#include <stdarg.h>
#include "open_drain_serial.h"
#include "devices.h"
#include <dw/debug_wire.h>

#define DW_SW_BRKPT_SIZE 10

typedef struct dw_sw_brkpt{
    uint16_t address;
    uint16_t opcode;

    uint8_t active : 1;
    uint8_t hwbp : 1;
} dw_sw_brkpt_t;

typedef struct debug_wire {
    uint32_t target_frequency;
    uint8_t cur_divisor;
    uint16_t program_counter;

    uint8_t halted : 1;
    uint8_t run_timers : 1;
    dw_device_definition_t device;

    dw_sw_brkpt_t swbrkpt[DW_SW_BRKPT_SIZE];
    uint8_t swbrkpt_n;
} debug_wire_t;

extern debug_wire_t debug_wire_g;
#define DW_DIVISOR_128              0x83
#define DW_DIVISOR_64               0x82
#define DW_DIVISOR_32               0x81
#define DW_DIVISOR_16               0x80
#define DW_DIVISOR_8                0xA0
#define DW_DIVISOR_4                0xA1
#define DW_DIVISOR_2                0xA2
#define DW_DIVISOR_1                0xA1


uint16_t dw_cmd_get_16(uint8_t cmd);
uint8_t dw_cmd_get_8(uint8_t cmd);
void dw_cmd_send_multiple(uint8_t command, void * data, uint8_t len);
void dw_cmd_send_multiple_consts(uint8_t command, uint16_t n, ...);

#define dw_cmd(cmd)                     od_uart_tx_byte(cmd)

#define DW_CMD_RESET                    0x07
#define DW_CMD_DISABLE                  0x06
#define DW_CMD_START_SRAM_CYCLE         0x20
#define DW_CMD_START_SRAM_CYCLE_SS      0x21
#define DW_CMD_EXECUTE_LOADED_INSTR     0x23
#define DW_CMD_GO                       0x30
#define DW_CMD_SS                       0x31
#define DW_CMD_GO_FROM_IR               0x32
#define DW_CMD_EXECUTE_LOADED_INSTR_LNG 0x33

uint8_t dw_cmd_halt(void);

//getters
#define DW_CMD_REG_PC                   0x00
#define DW_CMD_REG_HWBP                 0x01
#define DW_CMD_REG_IR                   0x02
#define DW_CMD_REG_SIGNATURE            0x03

#define dw_cmd_get(reg)                 (dw_cmd_get_16(0xF0 | reg))
#define dw_cmd_get_l(reg)               (dw_cmd_get_16(0xE0 | reg))

//setters
#define dw_cmd_multi(cmd, data_ptr, len)    (dw_cmd_send_multiple(cmd, data_ptr, len))
#define dw_cmd_set(reg, data_ptr)           (dw_cmd_multi((0xD0 | reg), data_ptr, 2))
#define dw_cmd_set_multi(reg, a, b)         (dw_cmd_send_multiple_consts((0xD0 | reg), 2, a, b))
#define dw_cmd_set_l(reg, data_ptr)         (dw_cmd_multi((0xC0 | reg), data_ptr, 1))

//reset/disable
#define dw_cmd_disable                  dw_cmd(DW_CMD_DISABLE)
void dw_cmd_reset(void);
uint8_t dw_cmd_set_speed(uint8_t divisor);

//flow control
#define dw_cmd_start_mem_cycle()        dw_cmd(DW_CMD_START_SRAM_CYCLE)
#define dw_cmd_start_mem_cycle_ss()     dw_cmd(DW_CMD_START_SRAM_CYCLE_SS)
#define dw_cmd_execute_loaded()         dw_cmd(DW_CMD_EXECUTE_LOADED_INSTR)
#define dw_cmd_ss(swbkpt)               (dw_cmd((swbkpt) ? DW_CMD_EXECUTE_LOADED_INSTR : DW_CMD_SS))
void dw_cmd_go(uint8_t is_sw_brkpt);

//contexts
#define DW_GO_CNTX_MEM                  0x66
#define DW_GO_CNTX_FLASH_WRT            0x64

#define dw_set_context(ctx)             dw_cmd(ctx)

//memory
#define DW_MODE_SRAM_READ               0x00
#define DW_MODE_REGS_READ               0x01
#define DW_MODE_FLASH_READ              0x02
#define DW_MODE_SRAM_WRITE              0x04
#define DW_MODE_REGS_WRITE              0x05

#define dw_cmd_mode_set(mode)           (dw_cmd_send_multiple_consts(0xC2, 1, mode))

void dw_ll_exec(uint16_t instruction, uint8_t long_instruction);
uint8_t dw_ll_register_read(uint8_t reg);
void dw_ll_register_write(uint8_t reg, uint8_t data);
void dw_ll_registers_read(uint8_t from, uint8_t to, void * buffer);
void dw_ll_registers_write(uint8_t from, uint8_t to, const void * buffer);
void dw_ll_sram_read(uint16_t from, uint16_t len, void * buffer);
void dw_ll_sram_write(uint16_t from, uint16_t len, void * buffer);
void dw_ll_flash_read(uint16_t from, uint16_t len, void * buffer);
void dw_ll_flash_clear_page(uint16_t address);

uint8_t dw_ll_flash_write_page_begin(uint16_t address);
uint8_t dw_ll_flash_write_populate_buffer(const uint16_t * buffer, uint16_t len, uint16_t remaining);
void dw_ll_flash_write_execute(void);

uint8_t dw_ll_eeprom_read_byte(uint16_t address);
void dw_ll_eeprom_read(void * buffer, uint16_t address, uint16_t len);
void dw_ll_eeprom_write_byte(uint16_t address, uint8_t data);
void dw_ll_eeprom_write(const void * buffer, uint16_t address, uint16_t len);

#endif //ARDWINO_DEBUG_WIRE_LL_H
