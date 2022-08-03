//
// Created by stefano on 03/08/22.
//
#include<avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "panic.h"

__attribute__((noreturn))
void panic(void){
    cli();
    DDRD |= (1 << PIND5);
    for(;;){
        PORTD ^= (1 << PIND5);
        _delay_ms(250);
    }
}