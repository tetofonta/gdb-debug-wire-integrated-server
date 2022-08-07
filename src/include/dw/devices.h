//
// Created by stefano on 03/08/22.
//

#ifndef ARDWINO_DEVICES_H
#define ARDWINO_DEVICES_H

#include <avr/io.h>
#include <avr/pgmspace.h>

typedef struct dw_device_definition{
    uint16_t signature;
    uint16_t sram_base;         //first sram address after io and extended io
    uint8_t flash_page_end;     //words per page
    uint16_t flash_end;         //last flash valid word
    uint16_t sram_end;          //first sram address after io and extended io
    uint16_t eeprom_end;        //first eeporom address after io and extended io
    uint8_t reg_spmcsr;
    uint8_t reg_dwdr;
} __attribute__((packed)) dw_device_definition_t;

struct dw_devices{
    uint16_t items;
    dw_device_definition_t devices[];
} __attribute__((packed));

extern const struct dw_devices devices PROGMEM;

#endif //ARDWINO_DEVICES_H
