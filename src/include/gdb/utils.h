//
// Created by stefano on 08/08/22.
//

#ifndef ARDWINO_UTILS_H
#define ARDWINO_UTILS_H
#include <stdint.h>
uint8_t hex2nib(char hex);
uint16_t byte2hex(uint8_t byte);
uint8_t nib2hex(uint8_t nib);
char * parse_hex_until(char character, char * buffer, uint16_t * len, uint64_t * out);
#endif //ARDWINO_UTILS_H
