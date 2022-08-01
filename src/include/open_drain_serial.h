//
// Created by stefano on 01/08/22.
//

#ifndef ARDWINO_OPEN_DRAIN_SERIAL_H
#define ARDWINO_OPEN_DRAIN_SERIAL_H

#include <avr/io.h>
#include <avr/interrupt.h>

#define OD_UART_RX_BUFFER_SIZE 8

extern volatile uint8_t uart_tx_buffer_full;
extern uint8_t uart_rx_buffer[8];
extern volatile uint8_t uart_rx_buffer_pointer;

uint8_t od_uart_busy(void);
void od_uart_init(uint32_t baud_rate);
void od_uart_tx(uint8_t data);

#endif
