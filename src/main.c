
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include <usb_cdc.h>
#include <debug_wire.h>

void cdc_task(void);

open_drain_pin_t dw_pin;
int main(void) {
    DDRB |= (1 << PINB7);
    PORTB |= (1 << PINB7);

    usb_cdc_init();
    open_drain_init(&dw_pin, &PIND, (1 << PIND7));
    open_drain_set_high(&dw_pin);

    dw_init(&dw_pin, 8000000);
    dw_set_speed(128);

    sei();

    for (;;) {
        cdc_task();
        USB_USBTask();
    }
}

uint8_t buffer[5];
uint8_t * a;
void cdc_task(void) {
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    uint16_t len = usb_cdc_read(buffer, 5);
    if(len > 0) {
        dw_send(buffer[0]);
        dw_recv_enable();
    }
}