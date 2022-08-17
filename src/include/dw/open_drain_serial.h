//
// Created by stefano on 01/08/22.
//

#ifndef ARDWINO_OPEN_DRAIN_SERIAL_H
#define ARDWINO_OPEN_DRAIN_SERIAL_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>

#define OD_UART_RX_BUFFER_SIZE 64

#define OD_UART_FLAG_BUSY_MASK      (1 << 6)
#define OD_UART_FLAG_WRT_MASK       (1 << 5)
#define OD_UART_FLAG_AVAIL_MASK     (1 << 4)

#define OD_UART_STATUS(mask)        (od_uart_status() & mask)
#define OD_UART_BUSY()              OD_UART_STATUS(OD_UART_FLAG_BUSY_MASK)
#define OD_UART_WRT()               OD_UART_STATUS(OD_UART_FLAG_WRT_MASK)
#define OD_UART_AVAIL()             OD_UART_STATUS(OD_UART_FLAG_AVAIL_MASK)
#define OD_UART_TX_FULL()           (uart_tx_buffer_full)
#define OD_UART_RX_FULL()           (uart_rx_buffer_pointer == OD_UART_RX_BUFFER_SIZE)
#define OD_UART_AVAILABLE()         (uart_rx_buffer_pointer)
#define OD_UART_DATA_PTR            (uart_rx_buffer)
#define OD_UART_DATA(item)          (*(OD_UART_DATA_PTR + item))

extern volatile uint8_t uart_tx_buffer_full;
extern volatile uint8_t uart_rx_buffer_pointer;
extern uint8_t uart_rx_buffer[OD_UART_RX_BUFFER_SIZE];

uint8_t od_uart_status(void);
void od_uart_init(uint32_t baud_rate);
void od_uart_deinit(void);
void od_uart_tx_byte(uint8_t data);
void od_uart_clear(void);
void od_uart_break(void);
void od_uart_blank(uint8_t frames);

void od_uart_send(const void *data, uint16_t len);
void od_uart_recv(void * buffer, uint16_t expected_len);
uint8_t od_uart_recv_byte(void);
uint8_t od_uart_recv_byte_timeout(uint16_t * timeout_ms);
void od_uart_wait_until(uint16_t to_read);
#define od_uart_discard(n) od_uart_recv(NULL, n)

inline void od_uart_irq_rx(uint8_t data){}
extern inline void od_uart_irq_break(void);
inline void od_uart_after_irq_rx(uint8_t data){}
extern inline void od_uart_after_irq_break(void);

#endif
