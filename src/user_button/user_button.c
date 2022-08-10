//
// Created by stefano on 03/08/22.
//

#include <stdbool.h>
#include "user_button.h"
#include "leds.h"

static uint8_t usr_pshbtn_debounce_counter = 0;
static uint8_t global_debounce = 0;
extern volatile bool connection_evt;

ISR(TIMER0_OVF_vect){
    if(connection_evt) {
        GDB_LED_OFF();
        connection_evt = 0;
    }

    if(usr_pshbtn_debounce_counter++ == 16) global_debounce = 0;
}

void usr_btn_init(user_button_state_t * btn, volatile uint8_t * pin_register, uint8_t pin){
    *(pin_register + 1) &= (1 << pin);
    *(pin_register + 2) |= (1 << pin);

    btn->pin_register = pin_register;
    btn->pin = pin;
    btn->last_state = (*pin_register) << pin != 0;
}

void usr_btn_setup(void){
    TCCR0A = 0;
    TCCR0B = (5 << CS00); //timer for debounce. 255 * (1024/16) us = 16ms
    TIMSK0 |= (1 << TOIE0);
}

void usr_btn_task(user_button_state_t * btn){
    if((*btn->pin_register) & (1 << btn->pin)){
        btn->last_state = 1;
    } else {
        if(btn->last_state && !global_debounce){
            usr_btn_event(btn);
            global_debounce = 1;
            usr_pshbtn_debounce_counter = 0;
            TCNT0 = 0;
        }
        btn->last_state = 0;
    }
}