//
// Created by stefano on 31/07/22.
//

#ifndef ARDWINO_DEBUG_WIRE_H
#define ARDWINO_DEBUG_WIRE_H

#include <stdint.h>

void dw_init(uint32_t target_freq);
void dw_set_speed(uint8_t divisor);
void dw_send(uint8_t byte);


#endif //ARDWINO_DEBUG_WIRE_H
