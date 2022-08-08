//
// Created by stefano on 08/08/22.
//

#include <gdb/utils.h>

uint8_t hex2nib(char hex){
    if(hex >= '0' && hex <= '9') return hex - '0';
    return hex - 'a' + 10;
}

uint8_t nib2hex(uint8_t nib){
    if(nib < 10) return nib + '0';
    return nib - 10 + 'a';
}

uint16_t byte2hex(uint8_t byte){
    return nib2hex(byte & 0x0F) << 8 | (nib2hex(byte >> 4)); //is little endian
}