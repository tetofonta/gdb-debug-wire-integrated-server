//
// Created by stefano on 13/08/22.
//
#include <stk500/stk500.h>
#include <util/delay.h>
#include "dw/open_drain.h"
#include "usb/usb_cdc.h"

void reset_target(bool reset) {
    if((reset && param.reset_active_high) || (!reset && !param.reset_active_high)) OD_HIGH(D, 7);
    else OD_LOW(D, 7);
}

// See AVR datasheets, chapter "SERIAL_PRG Programming Algorithm":
void enter_pgm_mode(void){
    PORTB &= ~(1 << PINB1); //sck low
    _delay_ms(20);
    reset_target(false);
    _delay_us(100);
    reset_target(true);
    _delay_ms(50);
    spi_transaction(0xAC, 0x53, 0x00, 0x00);
}

void exit_pgm_mode(void){
    spi_deinit();
    reset_target(false);
}

#define flash_read_addr(addr, high) (spi_transaction(0x20 + (high) * 8, ((addr) >> 8) & 0xFF, (addr) & 0xFF, 0))
#define eeprom_read_byte(addr)      (spi_transaction(0xA0, ((addr) >> 8) & 0xFF, (addr) & 0xFF, 0xFF))

bool isp_read_flash(uint16_t address, uint16_t length){
    uint16_t data;
    while(length--){
        data = flash_read_addr(address, 0) | (flash_read_addr(address, 1) << 8);
        usb_cdc_write(&data, 2);
        address++;
    }
    return true;
}

static uint16_t current_page(uint16_t address) {
    if (param.pagesize == 32) {
        return address & 0xFFF0;
    }
    if (param.pagesize == 64) {
        return address & 0xFFE0;
    }
    if (param.pagesize == 128) {
        return address & 0xFFC0;
    }
    if (param.pagesize == 256) {
        return address & 0xFF80;
    }
    return address;
}

#define isp_commit(page) (spi_transaction(0x4C, ((page) >> 8) & 0xFF, (page) & 0xFF, 0))
#define flash(high, addr, data) (spi_transaction(0x40 + 8 * (high), (addr) >> 8 & 0xFF, (addr) & 0xFF, data))

void isp_write_flash(uint16_t address, uint16_t length){
    unsigned int page = current_page(address);
    while(length--){
        if(page != current_page(address)){
            isp_commit(page);
            page = current_page(address);
        }

        uint8_t byte = usb_cdc_read_byte();
        flash(0, address, byte);
        byte = usb_cdc_read_byte();
        flash(1, address, byte);
        address++;
    }
    isp_commit(page);

    if (CRC_EOP == usb_cdc_read_byte()) {
        usb_cdc_write_PSTR(STK_INSYNC, 1);
        usb_cdc_write_PSTR(STK_OK, 1);
    } else
        usb_cdc_write_PSTR(STK_NOSYNC, 1);
}