//
// Created by stefano on 03/08/22.
//
#include <dw/devices/atmega328p.h>
#include <dw/devices/attiny85.h>

const struct dw_devices devices PROGMEM = {
        .items = 2,
        .devices = {
                DW_DEF_ATTINY85,
                DW_DEF_ATMEGA328,
        }
};