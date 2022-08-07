//
// Created by stefano on 03/08/22.
//

#ifndef ARDWINO_ATMEGA328P_H
#define ARDWINO_ATMEGA328P_H
#include <dw/devices.h>
//                                  sig     sram_base   sram_end    flash_page_end  flash_end   eeprom_end  spmcsr  dwdr  eearl eecr  eedr
#define DW_DEF_ATMEGA328 DW_DEV_DEF(0x0F95, 0x100,      0x08FF,     64,             0x3FFF,     0x03FF,     0x37,   0x31, 0x21, 0x1F, 0x20)

#endif //ARDWINO_ATMEGA328P_H
