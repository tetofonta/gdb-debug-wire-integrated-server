//
// Created by stefano on 17/08/22.
//

#ifndef ARDWINO_RTT_H
#define ARDWINO_RTT_H

#include <gdb/utils.h>
#include "dw/debug_wire_ll.h"

uint8_t rtt_is_available(void);
void rtt_set_state(uint8_t state);
uint8_t rtt_get_last_message(uint8_t * dest);

#endif //ARDWINO_RTT_H
