
#include <avr/io.h>
#include <avr/interrupt.h>

#include <usb/usb_cdc.h>
#include <dw/open_drain_serial.h>
#include <dw/debug_wire.h>
#include <dw/open_drain.h>
#include <panic.h>
#include "dw/devices.h"
#include "user_button.h"

static user_button_state_t rst_button;

void cdc_task(void);

int main(void) {
    usr_btn_setup();
    usr_btn_init(&rst_button, &PINB, 6);

    DDRD |= (1 << PIND5); //tx led
    DDRD |= (1 << PIND4); //rx led
    PORTD ^= (1 << PIND4);

    DDRB |= (1 << PINB4) | (1 << PINB7);
    PORTB |= (1 << PINB4);

    OD_HIGH(D, 7);

    usb_cdc_init();
    sei();

    _delay_ms(100);
    MUST_SUCCEED(dw_init(2*8000000), 1);
    debug_wire_device_reset();

    for (;;) {
        usr_btn_task(&rst_button);
        cdc_task();
        USB_USBTask();
    }
}

uint8_t buffer[32];

void cdc_task(void) {
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    uint16_t len = usb_cdc_read(buffer, 5);

    if (len > 0) {
        switch (buffer[0]) {

        }
    }
}

void usr_btn_event(user_button_state_t * btn){
    debug_wire_device_reset();
}