//
// Created by stefano on 10/08/22.
//

#ifndef ARDWINO_LEDS_H
#define ARDWINO_LEDS_H

#define GDB_LED_INIT()              do{DDRD |= (1 << PIND5);}while(0)
#define GDB_LED_ON()                do{PORTD &= ~(1 << PIND5);}while(0)
#define GDB_LED_OFF()               do{PORTD |= (1 << PIND5);}while(0)
#define GDB_LED_TOGGLE()            do{PORTD ^= (1 << PIND5);}while(0)
#define GDB_LED_SET(v)              do{PORTD ^= (PORTD ^ (v << PIND5)) & (1 << PIND5);}while(0)

#define DW_LED_INIT()               do{DDRD |= (1 << PIND4);}while(0)
#define DW_LED_ON()                 do{PORTD &= ~(1 << PIND4);}while(0)
#define DW_LED_OFF()                do{PORTD |= (1 << PIND4);}while(0)
#define DW_LED_TOGGLE()            do{PORTD ^= (1 << PIND4);}while(0)
#define DW_LED_SET(v)               do{PORTD ^= (PORTD ^ (v << PIND4)) & (1 << PIND4);}while(0)

#endif //ARDWINO_LEDS_H
