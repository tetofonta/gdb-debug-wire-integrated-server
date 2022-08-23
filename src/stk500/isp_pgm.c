//
// Created by stefano on 13/08/22.
//

#include <util/delay.h>
#include "isp/isp.h"
#include "usb/usb_cdc.h"
#include "gdb/gdb.h"
#include "leds.h"

void isp_deinit(void){
    spi_deinit();
    DDRD &= ~(1 << PIND7); //reset is an input
    PORTD |= (1 << PIND7); //pulled up

    GDB_LED_OFF();
    DW_LED_OFF();
}
void isp_init(void){
    spi_init();
    PORTD |= (1 << PIND7); //high
    DDRD |= (1 << PIND7); //reset is an output

    GDB_LED_ON();
    DW_LED_ON();
}

/**
 * Escape character 0x1B
 *
 * 0x01 -> begin_isp_mode -> pulse reset pin
 * 0xFE -> inverted reset pulse
 * 0x02 -> reset_low
 * 0x03 -> reset_high
 *
 * [ESC]<byte> => send byte
 */
void isp_task(void){
    if (Endpoint_IsOUTReceived()) {
        uint8_t cmd = usb_cdc_read_byte();
        uint8_t answ;
        switch(cmd){
            case 0x01:
            case 0xFE:
                //hold sck low and pulse reset
                PORTB &= ~(1 << PINB1);

                _delay_ms(20);
                PORTD ^= ((cmd & 1) << PIND7) ^ ((PORTD) & (1 << PIND7));// 0xxxxxxx 00000001
                _delay_us(100); //for cpu speed above 20khz 100us > 2clock cycles
                PORTD ^= ((~cmd & 1) << PIND7) ^ ((PORTD) & (1 << PIND7));
                _delay_ms(20);
                break;
            case 0x02:
                PORTD &= ~(1 << PIND7);
                break;
            case 0x03:
                PORTD |= (1 << PIND7);
                break;
            case 0x1B:
                cmd = usb_cdc_read_byte();
            default:
                answ = spi_transfer(cmd);
        }

        usb_cdc_write(&answ, 1);
    }
}

