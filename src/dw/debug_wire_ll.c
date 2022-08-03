//
// Created by stefano on 31/07/22.
//

#include <avr/io.h>
#include <util/delay.h>
#include <dw/debug_wire.h>
#include <dw/devices.h>
#include <panic.h>

struct debug_wire {
    uint32_t target_frequency;
    uint8_t cur_divisor;

    uint8_t status;
    uint8_t flags;

    dw_device_definition_t * device;
} debug_wire_g;

static volatile uint8_t waiting_break;

uint8_t dw_init(uint32_t target_freq) {
    debug_wire_g.target_frequency = target_freq;
    od_uart_clear();
    od_uart_init(target_freq >> 7);

    MUST_SUCCEED(dw_cmd_halt(), 1);
    uint16_t signature = dw_cmd_get(DW_CMD_REG_SIGNATURE);

    uint16_t len = pgm_read_word(&devices.items);
    dw_device_definition_t * devs = (dw_device_definition_t *) &devices.devices;
    while(len--){
        if(pgm_read_word(&devs->signature) == signature){
            debug_wire_g.device = devs;
            return 1;
        }
        devs++;
    }
    return 0;
}

uint8_t dw_cmd_halt(void){
    od_uart_clear();
    od_uart_break();
    uint16_t ret;
    od_uart_recv(&ret, 2);
    if((ret & 0xFF00) != 0x5500) return 0;
    debug_wire_g.status = DW_STATUS_HALTED;
    return dw_cmd_set_speed(DW_DIVISOR_128);
}

uint16_t dw_cmd_get_16(uint8_t cmd){
    uint16_t ret;
    dw_cmd(cmd);
    od_uart_recv(&ret, 2);
    return ret;
}
uint8_t dw_cmd_get_8(uint8_t cmd){
    uint8_t ret;
    dw_cmd(cmd);
    od_uart_recv(&ret, 1);
    return ret;
}
uint8_t dw_cmd_set_speed(uint8_t divisor){
    debug_wire_g.cur_divisor = divisor;
    return dw_cmd_get_8(divisor) == 0x55;
}

void dw_cmd_reset(void){
    od_uart_clear();
    waiting_break = 1;
    dw_cmd(DW_CMD_RESET);
    while(waiting_break);

    dw_init(debug_wire_g.target_frequency);
    od_uart_blank();
    od_uart_blank();
    dw_cmd(debug_wire_g.cur_divisor);
}

inline void od_uart_irq_rx(uint8_t data){}
inline void od_uart_irq_frame_error(void){

}
inline void od_uart_irq_break(void){
    if(!waiting_break) panic();
    waiting_break = 0;
}
