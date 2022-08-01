//
// Created by stefano on 31/07/22.
//

#include "open_drain.h"

void open_drain_init(open_drain_pin_t * pin, volatile uint8_t * pin_register, uint8_t mask){
    pin->pin_reg = pin_register;
    pin->mask = mask;
}

void open_drain_toggle(open_drain_pin_t * pin){
    *(pin->pin_reg + 1) ^= pin->mask;
    *(pin->pin_reg + 2) ^= pin->mask;
}

void open_drain_set_high(open_drain_pin_t * pin){
    *(pin->pin_reg + 1) &= ~pin->mask; //make input
    *(pin->pin_reg + 2) |= pin->mask; //set PORT to high
}

void open_drain_set_low(open_drain_pin_t * pin){
    *(pin->pin_reg + 2) &= ~pin->mask; //set PORT to low
    *(pin->pin_reg + 1) |= pin->mask; //make output
}

uint8_t open_drain_read(open_drain_pin_t * pin){
    return *pin->pin_reg & pin->mask;
}
