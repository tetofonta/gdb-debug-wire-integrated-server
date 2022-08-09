
#include <avr/io.h>
#include <avr/interrupt.h>

#include <usb/usb_cdc.h>
#include <dw/open_drain.h>
#include <panic.h>
#include <user_button.h>
#include "gdb/gdb.h"
#include "dw/debug_wire.h"

static user_button_state_t rst_button;


int main(void) {
    usr_btn_setup();
    usr_btn_init(&rst_button, &PINB, 6);

    DDRD |= (1 << PIND5); //tx led -> usb connected
    DDRD |= (1 << PIND4); //rx led -> halted
    PORTD |= (1 << PIND5);
    PORTD |= (1 << PIND4);

    DDRB |= (1 << PINB4) | (1 << PINB7);
    PORTB |= (1 << PINB4);

    OD_HIGH(D, 7);

    usb_cdc_init();
    sei();

    gdb_init();

    for (;;) {
        usr_btn_task(&rst_button);
        gdb_task();
        USB_USBTask();
    }
}

//uint8_t buffer[32];
//
//void cdc_task(void) {
//    if (USB_DeviceState != DEVICE_STATE_Configured)
//        return;
//
//    uint16_t len = usb_cdc_read(buffer, 32);
//
//    if (len > 0) {
//        switch (buffer[0]) {
//            case 0:
//                dw_ll_registers_read(0, 31, buffer);
//                usb_cdc_write(buffer, 32);
//                break;
//            case 1:
//                dw_ll_registers_write(0, 31, buffer);
//                break;
//            case 2:
//                dw_ll_sram_read(buffer[1], buffer[2], buffer+3);
//                usb_cdc_write(buffer+3, buffer[2]);
//                break;
//            case 3:
//                dw_ll_sram_write(buffer[1], buffer[2], buffer+3);
//                break;
//            case 4:
//                dw_ll_flash_read(buffer[1], buffer[2], NULL);
//                usb_cdc_write(uart_rx_buffer, buffer[2]);
//                od_uart_clear();
//                break;
//            case 5:{
//                uint16_t rem = dw_ll_flash_write_page_begin(buffer[1] * 2 * debug_wire_g.device.flash_page_end);
//                uint16_t op = 0xCFFF;
//                while((rem = dw_ll_flash_write_populate_buffer(&op, 1, rem)));
//                dw_ll_flash_write_execute();
//                dw_cmd_reset();
//                break;
//            }
//            case 6:
//                buffer[0] = dw_ll_eeprom_read_byte(buffer[1]);
//                usb_cdc_write(buffer, 1);
//                break;
//            case 7:
//                dw_ll_eeprom_write_byte(buffer[1], buffer[2]);
//                break;
//            case 255:
//                dw_cmd_halt();
//                break;
//        }
//    }
//}

void usr_btn_event(user_button_state_t * btn){
    debug_wire_device_reset();
    debug_wire_resume(DW_GO_CNTX_CONTINUE);
}