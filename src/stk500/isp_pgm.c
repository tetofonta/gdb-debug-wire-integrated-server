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

extern void mode_gdb(void);
uint8_t answ;
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
        DW_LED_OFF();
        uint8_t cmd = usb_cdc_read_byte();
        switch(cmd){
            case 0x02:
                PORTD &= ~(1 << PIND7);
                break;
            case 0x03:
                PORTD |= (1 << PIND7);
                break;
            case 0x04:
            case 0x05:
                GDB_LED_SET(cmd & 1);
                break;
            case 0xFF:
                isp_deinit();
                mode_gdb();
                return;
            case 0x1B: {
                uint16_t len = 0;
                while(len == 0) len = usb_cdc_read(&cmd, 1);
            }
            default:
                answ = spi_transfer(cmd);
        }

        usb_cdc_write(&answ, 1);
        DW_LED_ON();

        Endpoint_SelectEndpoint(CDC_RX_EPADDR);
        if(!Endpoint_IsReadWriteAllowed()){
            Endpoint_ClearOUT();
        }
    }
}

