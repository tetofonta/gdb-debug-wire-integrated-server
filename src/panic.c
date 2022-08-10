//
// Created by stefano on 03/08/22.
//
#include<avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "panic.h"
#include "leds.h"

volatile register uint8_t uart_data asm("r5");
volatile register uint8_t fast_flags asm("r6");

__attribute__((noreturn))
void panic(void){
    cli();
    GDB_LED_INIT();
    for(;;){
        GDB_LED_TOGGLE();
        _delay_ms(250);
    }
}