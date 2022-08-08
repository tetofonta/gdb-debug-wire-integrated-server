//
// Created by stefano on 31/07/22.
//

#ifndef ARDWINO_DEBUG_WIRE_H
#define ARDWINO_DEBUG_WIRE_H

struct dw_state{
    uint16_t pc, hwbp, ir, x, y, z;
    uint8_t reg0, reg1, reg24;
};

extern struct dw_state cur_state;

#define DW_GO_CNTX_CONTINUE             0x60
#define DW_GO_CNTX_HWBP                 0x61
#define DW_GO_CNTX_STEP_OUT             0x63
#define DW_GO_CNTX_STEP_INTO            0x79
#define DW_GO_CNTX_AUTO_STEP            0x79
#define DW_GO_CNTX_SWBP                 0x79
#define DW_GO_CNTX_SS                   0x7a

uint8_t debug_wire_halt(void);
uint8_t dw_init(uint32_t target_freq);
void debug_wire_device_reset(void);
void debug_wire_resume(uint8_t context);


#define DW_ENV_REG_EXEC                 0
#define DW_ENV_REG_IO                   1
#define DW_ENV_REG_FLASG_READ           2
#define DW_ENV_SRAM_RW                  2
#define DW_ENV_FLASH_CLR_PAGE           3
#define DW_ENV_EEPROM_RW                4
#define DW_ENV_FLASH_WRITE              5

void dw_env_open(uint8_t env_type);
void dw_env_close(uint8_t env_type);

#endif //ARDWINO_DEBUG_WIRE_H
