//
// Created by stefano on 31/07/22.
//

#ifndef ARDWINO_OPEN_DRAIN_H
#define ARDWINO_OPEN_DRAIN_H

#include <stdint.h>

typedef struct open_drain_pin{
    volatile uint8_t* pin_reg;
    uint8_t mask;
} open_drain_pin_t;

void open_drain_init(open_drain_pin_t * pin, volatile uint8_t * pin_register, uint8_t mask);
void open_drain_toggle(open_drain_pin_t * pin);
void open_drain_set_high(open_drain_pin_t * pin);
void open_drain_set_low(open_drain_pin_t * pin);
uint8_t open_drain_read(open_drain_pin_t * pin);

#endif //ARDWINO_OPEN_DRAIN_H
