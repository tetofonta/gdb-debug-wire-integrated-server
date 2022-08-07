//
// Created by stefano on 03/08/22.
//

#ifndef ARDWINO_ATMEGA328P_H
#define ARDWINO_ATMEGA328P_H
#include <dw/devices.h>

#define DW_DEF_ATMEGA328 { .signature = 0x0F95, .sram_base = 0x100, .flash_page_end = 64, .flash_end = 0x3FFF, .sram_end = 0x08FF, .eeprom_end = 0x03FF, .reg_spmcsr = 0x37, .reg_dwdr = 0x31}


#endif //ARDWINO_ATMEGA328P_H
