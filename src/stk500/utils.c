//
// Created by stefano on 13/08/22.
//
#include "isp/isp.h"

void spi_init(void) {
    PORTB &= ~(1 << PINB1);
    DDRB |= (1 << PINB2) | (1 << PINB1) | (1 << PINB0);
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0) | (1 << SPR1);
}

void spi_deinit(void) {
    DDRB &= ~(1 << PINB2) & ~(1 << PINB1);
    SPCR &= ~(1 << SPE);
}

uint8_t spi_transfer(uint8_t byte) {
    SPDR = byte;
    while(!(SPSR & (1<<SPIF)));
    return SPDR;
}