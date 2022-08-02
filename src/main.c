
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include <usb_cdc.h>
#include <open_drain_serial.h>
#include <debug_wire.h>

void cdc_task(void);

int main(void) {
    DDRD |= (1 << PIND5);

    DDRB |= (1 << PINB7);
    PORTB |= (1 << PINB7);

    DDRD &= ~(1 << PIND7);
    PORTD |= (1 << PIND7);

    usb_cdc_init();
    od_uart_init(62500);

    sei();

    for (;;) {
        cdc_task();
        USB_USBTask();
    }
}

uint8_t buffer[5];
uint8_t *a;

void cdc_task(void) {
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    uint16_t len = usb_cdc_read(buffer, 5);
    if (len > 0) {
        if (buffer[0] == 1) {
            usb_cdc_write(&OD_UART_AVAILABLE(), 1);
            usb_cdc_write(OD_UART_DATA_PTR, OD_UART_AVAILABLE());
        } else if (buffer[0] == 2) {
            od_uart_tx(0x82);
            while(OD_UART_BUSY());
            od_uart_init(125000);
        } else {
            usb_cdc_write(buffer, len);
            od_uart_tx(buffer[0]);
        }
    }

}