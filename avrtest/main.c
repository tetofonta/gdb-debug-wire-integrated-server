//
// Created by stefano on 08/08/22.
//
#include <avr/io.h>
#include <string.h>
#include <util/delay.h>

struct rtt_data{
    uint8_t available : 1;
    uint8_t enabled : 1;
    uint8_t size : 6;
    uint8_t data[64];
} __attribute__((packed));

volatile struct rtt_data rtt;
void rtt_log(const char * buffer, uint8_t size){
    if(!rtt.enabled) 
        return; 
    memcpy(rtt.data, buffer, size & 0x3F);
    rtt.available = 1;
    rtt.size = size & 0x3F;
    rtt.available = 1;
    asm("break");
    rtt.available = 0;
}

volatile unsigned char i = 0;

void toggle(uint8_t i){
    PORTB &= ~(1<<PINB5);
    if(! (i&1))
        PORTB |= (1 << PINB5);
}

__attribute__((noreturn)) int main(void){
    DDRB |= (1 << PINB5);	// PB1 is output
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