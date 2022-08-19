//
// Created by stefano on 17/08/22.
//
#include <gdb/rtt.h>
uint8_t gdb_rtt_enable;

struct rtt_data{
        uint8_t available : 1;
        uint8_t enabled : 1;
        uint8_t size : 6;
} __attribute__((packed));

static struct rtt_data rtt_get_flags(void){
    struct rtt_data flags;
    dw_ll_sram_read(debug_wire_g.device.sram_base, 1, &flags);
    return flags;
}

uint8_t rtt_is_available(void){
    struct rtt_data flags = rtt_get_flags();
    if(flags.available)
        return flags.size;
    return 0;
}

void rtt_set_state(uint8_t state){
    dw_ll_sram_write(debug_wire_g.device.sram_base, 1, &state); //todo srambase
    gdb_rtt_enable = state >> 1;
}

uint8_t rtt_get_last_message(uint8_t * dest){
    struct rtt_data flags = rtt_get_flags();
    if(!flags.available) return 0;
    dw_ll_sram_read(debug_wire_g.device.sram_base + 1, flags.size, dest);
    uint8_t out = 2;
    dw_ll_sram_write(debug_wire_g.device.sram_base, 1, &out);
    return flags.size;
}