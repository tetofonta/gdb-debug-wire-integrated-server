
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

    OD_HIGH(D, 7);

    usb_cdc_init();
    sei();

    uint16_t pc  = 0;
    MUST_SUCCEED(dw_init(16000000), 1);
    dw_full_reset();

    for (;;) {
        usr_btn_task(&rst_button);
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
            case 7:
                dw_cmd(debug_wire_g.cur_divisor);
                return;
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

void usr_btn_event(user_button_state_t * btn){
    dw_full_reset();
}