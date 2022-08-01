//
// Created by stefano on 31/07/22.
//

#include <avr/io.h>
#include <avr/interrupt.h>
#include "debug_wire.h"

struct debug_wire {
    uint8_t cur_send_byte;
    volatile uint8_t cur_bit_sent;

    uint8_t recv_buffer[32];
    volatile uint8_t recv_bits;
    volatile uint8_t start_bit;
    volatile uint8_t reading;

    uint32_t target_freq;
    open_drain_pin_t *pin;
    uint8_t presc;
} debug_wire_g;


void dw_init(open_drain_pin_t *pin, uint32_t target_freq) {
    debug_wire_g.pin = pin;
    debug_wire_g.target_freq = target_freq;
    TCCR0A = (2 << WGM00); // timer 0 in CTC mode: clear after compare a
    TCCR0B &= ~7; // disable timer
    TIMSK0 &= ~(1 << OCIE0A);
}

void dw_set_speed(uint8_t divisor) {
    uint32_t cycles_per_bit = (16000000 / debug_wire_g.target_freq * divisor);

    if (cycles_per_bit <= 255 ) { debug_wire_g.presc = 1; OCR0A = cycles_per_bit; }
    else if (cycles_per_bit / 8 <= 255) { debug_wire_g.presc = 2; OCR0A = cycles_per_bit >> 3; }
    else if (cycles_per_bit / 64 <= 255) { debug_wire_g.presc = 3; OCR0A = cycles_per_bit >> 6; }
    else if (cycles_per_bit / 256 <= 255) { debug_wire_g.presc = 4; OCR0A = cycles_per_bit >> 8; }
    else if (cycles_per_bit / 1024 <= 255) { debug_wire_g.presc = 5; OCR0A = cycles_per_bit >> 10; }

    TCCR0B |= debug_wire_g.presc; //start the timer
}

void dw_send(uint8_t byte){
    debug_wire_g.cur_send_byte = byte;
    debug_wire_g.cur_bit_sent = 0;
    debug_wire_g.reading = 0;

    TCNT0 = 0;
    TIMSK0 |= (1 << OCIE0A);
    while (debug_wire_g.cur_bit_sent < 0x0A);
}

void dw_recv_enable(void){
    EICRB = 0; //irq on low
    EIMSK = (1 << INT7); //pd7 is interrupt
}

void isr_send(void){
    if(debug_wire_g.cur_bit_sent == 9){
        TIMSK0 &= ~(1 << OCIE0A); //disable this irq
        open_drain_set_high(debug_wire_g.pin);
    }else if (debug_wire_g.cur_bit_sent == 0 || ((debug_wire_g.cur_send_byte >> (debug_wire_g.cur_bit_sent-1)) & 1) == 0)
        //lo start bit viene gestito qui perchè il primo irq può avvenire più dilatato dalla partenza perchè il prescaler è free running
        open_drain_set_low(debug_wire_g.pin);
    else
        open_drain_set_high(debug_wire_g.pin);
    debug_wire_g.cur_bit_sent++;
}

void isr_recv(void){
    uint8_t value = PIND & (1 << PIND7);
    if(!debug_wire_g.start_bit){ //its not a start bit and it is a one
        PORTB ^= (1<<PINB7);
        debug_wire_g.recv_bits++;
    }else{
        debug_wire_g.start_bit = 0;
    }

    if(debug_wire_g.recv_bits == 9){ //byte completed
        TIMSK0 &= ~(1 << OCIE0A);
        PORTB ^= (1 << PINB7);
        debug_wire_g.recv_bits = 8;
        dw_recv_enable();
    }
}

ISR(INT7_vect){
    PORTB &= ~(1 << PINB7);

    EIMSK &= ~(1 << INT7);//disable start bit detection
    TCNT0 = 0;
    TIMSK0 |= (1 << OCIE0A);
    debug_wire_g.reading = 1;
    debug_wire_g.start_bit = 1;
}


ISR(TIMER0_COMPA_vect){
    if(!debug_wire_g.reading) isr_send();
    else isr_recv();
}

