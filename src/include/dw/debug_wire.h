//
// Created by stefano on 31/07/22.
//

#ifndef ARDWINO_DEBUG_WIRE_H
#define ARDWINO_DEBUG_WIRE_H

#include <stdint.h>
#include "open_drain_serial.h"
#include "devices.h"

typedef struct debug_wire {
    uint32_t target_frequency;
    uint8_t cur_divisor;

    uint8_t status;
    uint8_t flags;

    dw_device_definition_t * device;
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

#define DW_STATUS_HALTED    0
#define DW_STATUS_RUNNING   255

#define DW_FLAG_RUN_TIMERS  (1 << 5)

uint8_t dw_init(uint32_t target_freq);
uint16_t dw_cmd_get_16(uint8_t cmd);
uint8_t dw_cmd_get_8(uint8_t cmd);
void dw_cmd_send_multiple(uint8_t command, void * data, uint8_t len);


#define dw_cmd(cmd)                     do{ while(OD_UART_TX_FULL()); od_uart_tx_byte(cmd); }while(0);

#define DW_CMD_REG_PC                   0x00
#define DW_CMD_REG_HWBP                 0x01
#define DW_CMD_REG_IR                   0x02
#define DW_CMD_REG_SIGNATURE            0x03

#define DW_CMD_RESET                    0x07
#define DW_CMD_DISABLE                  0x06
#define DW_CMD_START_SRAM_CYCLE         0x20
#define DW_CMD_START_SRAM_CYCLE_SS      0x21
#define DW_CMD_EXECUTE_LOADED_INSTR     0x23
#define DW_CMD_GO                       0x30
#define DW_CMD_SS                       0x31

#define DW_GO_CNTX_CONTINUE             0x60
#define DW_GO_CNTX_HWBP                 0x61
#define DW_GO_CNTX_STEP_OUT             0x63
#define DW_GO_CNTX_STEP_INTO            0x79
#define DW_GO_CNTX_AUTO_STEP            0x79
#define DW_GO_CNTX_SWBP                 0x79
#define DW_GO_CNTX_SS                   0x7a
#define DW_GO_CNTX_MEM                  0x66
#define DW_GO_CNTX_FLASH_WRT            0x64


uint8_t dw_cmd_halt(void);

//getters
#define dw_cmd_get(reg)                 (dw_cmd_get_16(0xF0 | reg))
#define dw_cmd_get_l(reg)               (dw_cmd_get_16(0xE0 | reg))


//setters
#define dw_cmd_multi_const(cmd, len, data...)      do{uint8_t d[] = {data}; dw_cmd_send_multiple(cmd, d, len);}while(0)

#define dw_cmd_multi(cmd, data_ptr, len)    (dw_cmd_send_multiple(cmd, data_ptr, len))
#define dw_cmd_set(reg, data_ptr)           (dw_cmd_multi((0xD0 | reg), data_ptr, 2))
#define dw_cmd_set_l(reg, data_ptr)         (dw_cmd_multi((0xD0 | reg), data_ptr, 1))

//reset/disable
#define dw_cmd_disable                  dw_cmd(DW_CMD_DISABLE)
void dw_cmd_reset(void);
uint8_t dw_cmd_set_speed(uint8_t divisor);

//flow control
#define dw_cmd_start_sram_cycle()       dw_cmd(DW_CMD_START_SRAM_CYCLE)
#define dw_cmd_start_sram_cycle_ss()    dw_cmd(DW_CMD_START_SRAM_CYCLE_SS)
#define dw_cmd_execute_loaded           dw_cmd(DW_CMD_EXECUTE_LOADED_INSTR)
#define dw_cmd_go()                     dw_cmd(DW_CMD_GO)
#define dw_cmd_ss()                     dw_cmd(DW_CMD_SS)

//contexts
#define dw_set_context(ctx)             dw_cmd(ctx)

#endif //ARDWINO_DEBUG_WIRE_H
