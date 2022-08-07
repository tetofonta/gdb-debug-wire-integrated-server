//
// Created by stefano on 03/08/22.
//

#ifndef ARDWINO_ATTINY85_H
#define ARDWINO_ATTINY85_H
#include <dw/devices.h>
#define DW_DEF_ATTINY85 { .signature = 0x0B93, .sram_base = 0x60, .flash_page_end = 32, .flash_end = 0x0FFF, .sram_end = 0x025F, .eeprom_end = 0x1FF, .reg_spmcsr = 0x37, .reg_dwdr = 0x22 }
#endif //ARDWINO_ATTINY85_H
