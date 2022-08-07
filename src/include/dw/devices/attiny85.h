//
// Created by stefano on 03/08/22.
//

#ifndef ARDWINO_ATTINY85_H
#define ARDWINO_ATTINY85_H
#include <dw/devices.h>

//                                  sig     sram_base   sram_end    flash_page_end  flash_end   eeprom_end  spmcsr  dwdr  eearl eecr  eedr
#define DW_DEF_ATTINY85  DW_DEV_DEF(0x0B93, 0x60,      0x025F,      32,             0x0FFF,     0x1FF,      0x37,   0x22, 0x1E, 0x1C, 0x1D)

#endif //ARDWINO_ATTINY85_H
