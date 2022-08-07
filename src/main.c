
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
            case 0: {
                dw_ll_read_flash(debug_wire_g.device.flash_page_end*2*buffer[1], debug_wire_g.device.flash_page_end*2, NULL);
                usb_cdc_write(uart_rx_buffer, debug_wire_g.device.flash_page_end*2);
                od_uart_clear();
                break;
            } case 1: {
                dw_ll_clear_flash_page(buffer[1] * debug_wire_g.device.flash_page_end*2);
                dw_cmd_reset();
                break;
            } case 3: {
                uint16_t rem = dw_ll_write_flash_page_begin(buffer[1] * debug_wire_g.device.flash_page_end*2);
                uint16_t a = 0xcfff;
                for (uint16_t i = 0; i < debug_wire_g.device.flash_page_end; ++i) {
                    a = 255 - i;
                    rem = dw_ll_write_flash_populate_buffer(&a, 1, rem);
                    if(rem == 0) break;
                }
                dw_ll_write_flash_execute();
                break;
            } case 4:
                dw_cmd_reset();
                break;
            case 5:
                dw_cmd_get(DW_CMD_REG_PC);
                break;
            case 6:
                dw_cmd_set_multi(DW_CMD_REG_PC, buffer[1], buffer[2]);
                break;
            case 0xf0: {
                dw_cmd(DW_CMD_DISABLE);
                break;
            }case 254: {
                dw_cmd_halt();
                break;
            } case 255:
                if(uart_rx_buffer_pointer)
                    usb_cdc_write(uart_rx_buffer, uart_rx_buffer_pointer);
                od_uart_clear();
                break;
        }
    }
}

void usr_btn_event(user_button_state_t * btn){
    debug_wire_device_reset();
}