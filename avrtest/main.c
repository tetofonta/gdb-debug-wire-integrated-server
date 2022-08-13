//
// Created by stefano on 08/08/22.
//
#include <avr/io.h>
#include <util/delay.h>

volatile unsigned char i = 0;

void toggle(uint8_t i){
    PORTB &= ~(1<<PINB5);
    if(i&1)
        PORTB |= (1 << PINB5);
}

__attribute__((noreturn)) int main(void){
    DDRB |= (1 << PINB5);	// PB1 is output
    PORTB &= ~(1 << PINB5); //off

    while(1){
        toggle(i);
        if(i % 4)
            _delay_ms(250);
        else
            _delay_ms(1000);
        i++;
    }
}