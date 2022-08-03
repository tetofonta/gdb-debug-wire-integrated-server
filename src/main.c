
#include <avr/io.h>
#include <avr/interrupt.h>

#include <usb/usb_cdc.h>
#include <dw/open_drain_serial.h>
#include <dw/debug_wire.h>
#include <dw/open_drain.h>
#include <panic.h>
#include "dw/devices.h"

static uint8_t usr_pshbtn_last_state = 0;
static uint8_t usr_pshbtn_debounce = 0;
static uint8_t usr_pshbtn_debounce_counter = 0;

ISR(TIMER0_OVF_vect){
    if(usr_pshbtn_debounce_counter++ == 16) usr_pshbtn_debounce = 0;
}

void cdc_task(void);
void usr_pshbtn_reset(void);

int main(void) {
    DDRB &= (1 << PINB6);
    PORTB |= (1 << PINB6); //reset button

    DDRD |= (1 << PIND5); //tx led
    DDRD |= (1 << PIND4); //rx led

    OD_HIGH(D, 7);

    TCCR0A = 0;
    TCCR0B = (5 << CS00); //timer for debounce. 255 * (1024/16) us = 16ms
    TIMSK0 |= (1 << TOIE0);

    usb_cdc_init();
    sei();
    MUST_SUCCEED(dw_init(16000000), 1);

    for (;;) {
        if((PINB & (1 << PINB6))){
            usr_pshbtn_last_state = 1;
        } else {
            if(usr_pshbtn_last_state && !usr_pshbtn_debounce){//if falling edge
                usr_pshbtn_reset();
                usr_pshbtn_debounce = 1;
                usr_pshbtn_debounce_counter = 0;
                TCNT0 = 0;
            }
            usr_pshbtn_last_state = 0;
        }

        cdc_task();
        USB_USBTask();
    }
}

uint8_t buffer[5];
uint8_t *a;
uint16_t answ;

void cdc_task(void) {
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    uint16_t len = usb_cdc_read(buffer, 5);
    if (len > 0) {
        switch (buffer[0]) {
            case 1:
                answ = dw_cmd_get(DW_CMD_REG_PC); break;
            case 2:
                answ = dw_cmd_get(DW_CMD_REG_HWBP); break;
            case 3:
                answ = dw_cmd_get(DW_CMD_REG_IR); break;
            case 4:
                answ = dw_cmd_get(DW_CMD_REG_SIGNATURE); break;
            case 5:
                dw_cmd_reset(); return;
            case 6:
                dw_cmd_halt(); return;
            case 255:
                usb_cdc_write(&uart_rx_buffer_pointer, 1);
                usb_cdc_write(uart_rx_buffer, uart_rx_buffer_pointer);
                return;
            default: {
                return;
            };
        }
        usb_cdc_write(&answ, 2);
    }
}

void usr_pshbtn_reset(void){

}