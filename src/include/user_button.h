//
// Created by stefano on 03/08/22.
//

#ifndef ARDWINO_USER_BUTTON_H
#define ARDWINO_USER_BUTTON_H

#include <avr/io.h>
#include <avr/interrupt.h>

typedef struct user_button_state{
    volatile uint8_t * pin_register;
    uint8_t last_state : 1;
    uint8_t pin : 3;
} user_button_state_t;

void usr_btn_init(user_button_state_t * btn, volatile uint8_t * pin_register, uint8_t pin);
void usr_btn_setup(void);
void usr_btn_task(user_button_state_t * btn);
extern void usr_btn_event(user_button_state_t * btn);

#endif //ARDWINO_USER_BUTTON_H
