//
// Created by stefano on 08/08/22.
//
#include <avr/io.h>
#include <util/delay.h>
#include <rtt.h>

char * data = "ON";
volatile unsigned char i = 0;

void toggle(uint8_t i){
    PORTB &= ~(1<<PINB5);
    if(! (i&1)){
        rtt_log(data, 2);
        PORTB |= (1 << PINB5);
    }
}

__attribute__((noreturn)) int main(void){
    rtt_init();

    DDRB |= (1 << PINB5); // PB1 is output
    PORTB &= ~(1 << PINB5); //off
    rtt_log("ciao", 4);

    while(1){
        toggle(i);
        if(i % 4)
            _delay_ms(250);
        else
            _delay_ms(1000);
        i++;
    }
}