//
// Created by stefano on 13/08/22.
//
#include <stk500/stk500.h>

void spi_init(void) {
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

uint8_t spi_transaction(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    spi_transfer(a);
    spi_transfer(b);
    spi_transfer(c);
    return spi_transfer(d);
}