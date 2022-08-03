//
// Created by stefano on 03/08/22.
//

#ifndef ARDWINO_DEVICES_H
#define ARDWINO_DEVICES_H

#include <avr/io.h>
#include <avr/pgmspace.h>

typedef struct dw_device_definition{
    uint16_t signature;
} __attribute__((packed)) dw_device_definition_t;

struct dw_devices{
    uint16_t items;
    dw_device_definition_t devices[];
} __attribute__((packed));

extern const struct dw_devices devices PROGMEM;

#endif //ARDWINO_DEVICES_H
