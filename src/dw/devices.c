//
// Created by stefano on 03/08/22.
//
#include <dw/devices/atmega328p.h>

const struct dw_devices devices PROGMEM = {
        .items = 1,
        .devices = {
                DW_DEF_ATMEGA328
        }
};