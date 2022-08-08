//
// Created by stefano on 08/08/22.
//
#include <avr/io.h>
#include <util/delay.h>

__attribute__((noreturn)) int main(void){
    DDRB |= (1 << PINB5);	// PB1 is output
    PORTB &= ~(1 << PINB5); //off

    unsigned char i = 0;
    while(1){
        PORTB |= (1 << PINB5); //on
        if(i % 2)
            _delay_ms(250);
        else
            _delay_ms(1000);
        PORTB &= ~(1 << PINB5);
        _delay_ms(500);
        i++;
    }
}