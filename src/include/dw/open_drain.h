//
// Created by stefano on 02/08/22.
//

#ifndef ARDWINO_OPEN_DRAIN_H
#define ARDWINO_OPEN_DRAIN_H

#include <avr/io.h>

#define _DDR(letter)                DDR ## letter
#define _PORT(letter)               PORT ## letter
#define _PIN(letter, pin)           PIN ## letter ## pin
#define _PIN_REG(letter)            PIN ## letter

#define OD_LOW(letter, pin)         do{_PORT(letter) &= ~(1 << _PIN(letter, pin)); _DDR(letter) |= (1 << _PIN(letter, pin));}while(0)
#define OD_HIGH(letter, pin)        do{_DDR(letter) &= ~(1 << _PIN(letter, pin)); _PORT(letter) |= (1 << _PIN(letter, pin));}while(0)
#define OD_TOGGLE(letter, pin)      do{_DDR(letter) ^= (1 << _PIN(letter, pin)); _PORT(letter) ^= (1 << _PIN(letter, pin));}while(0)
#define OD_READ(letter, pin)        (_PIN_REG(letter) & (1 << _PIN(letter, pin)))

#endif //ARDWINO_OPEN_DRAIN_H
