
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

    uint16_t pc  = 0;
    MUST_SUCCEED(dw_init(16000000), 1);
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
            case 1:
                debug_wire_halt();
                PORTD &= ~(1 << PIND4);
                break;
            case 2:
                debug_wire_resume(DW_GO_CNTX_CONTINUE); break;
            case 3:
                debug_wire_resume(DW_GO_CNTX_SS); break;
            case 4:
                debug_wire_g.swbrkpt[0].address = 0x0200;
                debug_wire_g.swbrkpt[0].instruction = 0x259a; //SBI PORTB, 5
                debug_wire_g.swbrkpt_n = 1;
                debug_wire_g.program_counter = 0x0200;
                dw_cmd_send_multiple_consts((DW_CMD_REG_PC | 0xD0), 2, 0, 2);
                dw_cmd_get(DW_CMD_REG_PC);
                debug_wire_resume(DW_GO_CNTX_SS);
                break;
            case 5:
                dw_cmd_get(DW_CMD_REG_PC);
                dw_cmd_get(DW_CMD_REG_IR);
            case 6: {
                uint8_t len = buffer[1];
                debug_wire_read_registers(buffer, len, 30);
                usb_cdc_write(buffer, len);
                break;
            } case 7: {
                uint8_t len = buffer[1];
                debug_wire_write_registers(buffer + 2, len, 30);
                usb_cdc_write(buffer + 2, len);
                break;
            } case 9: {
                uint8_t len = buffer[1];
                debug_wire_read_sram(buffer, len, 0x24);
                usb_cdc_write(buffer, len);
                break;
            } case 255:
                usb_cdc_write(&uart_rx_buffer_pointer, 1);
                usb_cdc_write(uart_rx_buffer, uart_rx_buffer_pointer);
                break;
        }
    }
}

void usr_btn_event(user_button_state_t * btn){
    debug_wire_device_reset();
}