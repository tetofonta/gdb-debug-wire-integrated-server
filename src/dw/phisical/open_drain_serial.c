//
// Created by stefano on 01/08/22.
//http://ww1.microchip.com/downloads/en/Appnotes/AVR274.pdf

#include <util/delay.h>
#include <string.h>
#include "dw/open_drain_serial.h"
#include "dw/open_drain.h"

#define TIMER_IRQ_ENABLE() TIMSK1 |= (1 << OCIE1A)
#define TIMER_IRQ_DISABLE() TIMSK1 &= ~(1 << OCIE1A)
#define TIMER_IRQ_CLEAR() TIFR1 |= (1 << OCF1A)
#define FE_IRQ_ENABLE() EIMSK |= (1 << INT7)
#define FE_IRQ_DISABLE() EIMSK &= ~(1 << INT7)
#define FE_IRQ_CLEAR() EIFR &= ~(1 << INT7)

#define _OD_UART_WRT      (fast_flags & OD_UART_FLAG_WRT_MASK)
#define _OD_UART_BUSY     (uart_flags & OD_UART_FLAG_BUSY_MASK)
#define _OD_UART_AVAIL    (uart_flags & OD_UART_FLAG_AVAIL_MASK)

volatile register uint8_t uart_data asm("r2");
volatile register uint8_t fast_flags asm("r3"); // (nextval) (    ) (wrt) (     ) (cnt3)(cnt2)(cnt1)(cnt0)
volatile uint8_t uart_flags;

volatile uint8_t uart_tx_buffer_full;
uint8_t  uart_tx_buffer;
uint8_t  uart_rx_buffer[OD_UART_RX_BUFFER_SIZE];
uint8_t  volatile uart_rx_buffer_pointer;

static uint8_t _od_uart_calc_prescaler(uint32_t baud_rate){
    uint16_t prescalers[] = {1, 8, 64, 256, 1024};
    for (int i = 0; i < 5; ++i) {
        uint32_t ocr_val = F_CPU / (baud_rate * prescalers[i]) - 1;
        if(ocr_val < 65535) {
            OCR1A = ocr_val;
            return i + 1;
        }
    }
    return 0;
}

inline void od_uart_init(uint32_t baud_rate){
    //setup counter
    TIMER_IRQ_DISABLE();
    FE_IRQ_DISABLE();

    TCCR1A = (0 << COM1A0) | (0 << COM1B0) | (0 << WGM10);
    TCCR1B = (1 << WGM12) | (_od_uart_calc_prescaler(baud_rate) << CS10);
    EICRB |= (0 << ISC70);

    fast_flags = 0;
    uart_flags = 0;
    uart_rx_buffer_pointer = 0;

    TCNT1 = 0;
    TIMER_IRQ_CLEAR();
    FE_IRQ_CLEAR();
    FE_IRQ_ENABLE();
}

uint8_t od_uart_status(void){
    return (fast_flags | uart_flags) & 0xF0;
}

void od_uart_tx_byte(uint8_t data){
    while(OD_UART_TX_FULL());
    uart_tx_buffer_full = 1;
    uart_tx_buffer = data;

    while(_OD_UART_BUSY && !_OD_UART_WRT);
    if(_OD_UART_BUSY) return; //already transmitting
    PORTB ^= (1 << PINB7);

    FE_IRQ_DISABLE();
    uart_tx_buffer_full = 0;
    uart_data = uart_tx_buffer;

    fast_flags &= ~(1 << 7);
    fast_flags |= OD_UART_FLAG_WRT_MASK;
    uart_flags |= OD_UART_FLAG_BUSY_MASK;

    OD_HIGH(D, 7);

    TCNT1 = 0;
    TIMER_IRQ_ENABLE();
}



void od_uart_clear(void){
    uart_rx_buffer_pointer = 0;
    uart_flags = 0;
}

static void _line_clear(uint8_t frames){
    TIMER_IRQ_CLEAR();
    TIMER_IRQ_ENABLE();
    uint8_t count = 10 * frames;
    while(count--){
        while(! (TIFR1 & (1 << OCF1A)));
        TIMER_IRQ_CLEAR();
    }
    TIMER_IRQ_DISABLE();
    FE_IRQ_CLEAR();
}

void od_uart_blank(uint8_t frames){
    cli();
    OD_HIGH(D, 7);
    _line_clear(frames);
    FE_IRQ_CLEAR();
    sei();
}

void od_uart_break(void){
    cli();
    OD_LOW(D, 7);
    _line_clear(2);
    OD_HIGH(D, 7);
    FE_IRQ_CLEAR();
    sei();
}

void od_uart_send(void * data, uint16_t len){
    while (len--){
        while(uart_tx_buffer_full);
        od_uart_tx_byte((uint8_t) *( (uint8_t*) data++));
    }
}

void od_uart_wait_until(uint16_t to_read){
    while(!_OD_UART_AVAIL || uart_rx_buffer_pointer < to_read);
}

void od_uart_recv(void * buffer, uint16_t expected_len){
    od_uart_clear();
    while(expected_len > 0){
        uint8_t to_read = expected_len < OD_UART_RX_BUFFER_SIZE ? expected_len : OD_UART_RX_BUFFER_SIZE;
        od_uart_wait_until(to_read);
        if(buffer != NULL){
            memcpy(buffer, uart_rx_buffer, to_read);
            buffer += to_read;
        }
        expected_len -= to_read;
        uart_rx_buffer_pointer -= to_read;
    }
}

uint8_t od_uart_recv_byte(void){
    od_uart_clear();
    while(uart_rx_buffer_pointer < 1);
    return *uart_rx_buffer;
}


__attribute__((optimize("-Ofast"))) ISR(TIMER1_COMPA_vect){
    cli();
    PORTB ^= (1 << PINB7);
    register uint8_t value = PIND;
    if(_OD_UART_WRT){
        if ((PORTD ^ fast_flags) & (1 << PIND7) )
            OD_TOGGLE(D, 7);
        register uint8_t v = ++fast_flags & 15;
        if(v > 9 ){
            fast_flags = 0;
            uart_flags &= ~(OD_UART_FLAG_BUSY_MASK);
            if(OD_UART_TX_FULL()){
                uart_data = uart_tx_buffer;
                fast_flags |= OD_UART_FLAG_WRT_MASK;
                uart_flags |= OD_UART_FLAG_BUSY_MASK;
                uart_tx_buffer_full = 0;
            } else {
                TIMER_IRQ_DISABLE();
                FE_IRQ_CLEAR();
                FE_IRQ_ENABLE();
            }
            sei();
            return;
        } else if(v == 9){
            fast_flags |= (1 << PIND7);
        } else {
            fast_flags &= ~(1 << PIND7);
            fast_flags |= uart_data << PIND7;
            uart_data >>= 1;
        }
    } else {
        register uint8_t v = ++fast_flags & 15;
        uart_data >>= 1;
        uart_data |= value & (1 << PIND7);
        if( v == 8) {
            PORTB ^= (1 << PINB4);
            if(uart_rx_buffer_pointer == OD_UART_RX_BUFFER_SIZE)
                uart_rx_buffer_pointer = 0;
            uart_rx_buffer[uart_rx_buffer_pointer++] = uart_data;
        } else if (v > 8){
            fast_flags = 0;
            uart_flags = OD_UART_FLAG_AVAIL_MASK;
            TIMER_IRQ_DISABLE();
            TIMER_IRQ_CLEAR();
            if(value & (1 << PIND7))
                od_uart_irq_rx(uart_data);
            else
                od_uart_irq_break();
            FE_IRQ_ENABLE();
            FE_IRQ_CLEAR();
            sei();
        }
    }
    sei();
}

__attribute__((optimize("-Ofast"))) ISR(INT7_vect){
    cli();
    FE_IRQ_DISABLE();
    PORTB ^= (1 << PINB7);
    fast_flags = 0;
    uart_flags |= OD_UART_FLAG_BUSY_MASK;
    TCNT1 = OCR1A >> 2;
    TIMER_IRQ_ENABLE();
    TIMER_IRQ_CLEAR();
    sei();
}
