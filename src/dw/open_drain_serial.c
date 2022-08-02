//
// Created by stefano on 01/08/22.
//http://ww1.microchip.com/downloads/en/Appnotes/AVR274.pdf

#include "open_drain_serial.h"

#define TIMER_IRQ_ENABLE() TIMSK1 |= (1 << OCIE1A)
#define TIMER_IRQ_DISABLE() TIMSK1 &= ~(1 << OCIE1A)
#define TIMER_IRQ_CLEAR() TIFR1 = 0
#define FE_IRQ_ENABLE() EIMSK |= (1 << INT7)
#define FE_IRQ_DISABLE() EIMSK &= ~(1 << INT7)
#define FE_IRQ_CLEAR() EIFR &= ~(1 << INT7)

#define _OD_UART_BUSY     (flags & OD_UART_FLAG_BUSY_MASK)
#define _OD_UART_WRT      (flags & OD_UART_FLAG_WRT_MASK)
#define _OD_UART_AVAIL    (flags & OD_UART_FLAG_AVAIL_MASK)

volatile register uint8_t uart_data asm("r5");
volatile register uint8_t flags asm("r6"); // (nextval) (busy) (wrt) (avail) (cnt3)(cnt2)(cnt1)(cnt0)
volatile uint8_t uart_tx_buffer_full;
uint8_t  uart_tx_buffer;
uint8_t  uart_rx_buffer[OD_UART_RX_BUFFER_SIZE];
uint8_t  volatile uart_rx_buffer_pointer;

uint8_t _od_uart_calc_prescaler(uint32_t baud_rate){
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

void od_uart_init(uint32_t baud_rate){

    //setup counter
    TCCR1A = (0 << COM1A0) | (0 << COM1B0) | (0 << WGM10);
    TCCR1B = (1 << WGM12) | (_od_uart_calc_prescaler(baud_rate) << CS10);
    EICRB |= (0 << ISC70);

    FE_IRQ_CLEAR();
    FE_IRQ_ENABLE();
    TIMER_IRQ_DISABLE();
}

uint8_t od_uart_status(void){
    return flags & 0xF0;
}

void od_uart_tx(uint8_t data){
    if(OD_UART_TX_FULL()) return;
    uart_tx_buffer_full = 1;
    uart_tx_buffer = data;

    if(_OD_UART_BUSY) return;

    FE_IRQ_DISABLE();
    uart_tx_buffer_full = 0;
    uart_data = uart_tx_buffer;
    flags |= OD_UART_FLAG_BUSY_MASK | OD_UART_FLAG_WRT_MASK;

    DDRD &= ~(1 << PIND7);
    PORTD |= (1 << PIND7);

    TCNT1 = 0;
    TIMER_IRQ_ENABLE();
}

ISR(TIMER1_COMPA_vect){
    register uint8_t value = PIND;
    if(_OD_UART_WRT){
        if ((PORTD ^ flags) & (1 << PIND7) ){
            DDRD ^= (1 << PIND7);
            PORTD ^= (1 << PIND7); //write last calculated value
        }
        register uint8_t v = ++flags & 15;
        if(v > 9 ){
            flags = 0;
            if(OD_UART_TX_FULL()){
                uart_data = uart_tx_buffer;
                flags |= OD_UART_FLAG_BUSY_MASK | OD_UART_FLAG_WRT_MASK;
                uart_tx_buffer_full = 0;
            } else {
                TIMER_IRQ_DISABLE();
                FE_IRQ_CLEAR();
                FE_IRQ_ENABLE();
            }
            sei();
            return;
        } else if(v == 9){
            flags |= (1 << PIND7);
        } else {
            flags &= ~(1 << PIND7);
            flags |= uart_data << PIND7;
            uart_data >>= 1;
        }
    } else {
        register uint8_t v = ++flags & 15;
        if(v <= 9) {
            uart_data >>= 1;
            uart_data |= value & (1 << PIND7);
        } if( v > 9){
            flags = OD_UART_FLAG_AVAIL_MASK;
            uart_rx_buffer[uart_rx_buffer_pointer++] = uart_data;
            if(uart_rx_buffer_pointer == OD_UART_RX_BUFFER_SIZE)
                uart_rx_buffer_pointer = 0;
            TIMER_IRQ_DISABLE();
            FE_IRQ_CLEAR();
            FE_IRQ_ENABLE();
        }
    }
}

ISR(INT7_vect){
    FE_IRQ_DISABLE();
    flags |= OD_UART_FLAG_BUSY_MASK;
    flags &= ~OD_UART_FLAG_WRT_MASK;
    TIMER_IRQ_ENABLE();
    TCNT1 = OCR1A - 1;
    //asm volatile("jmp __vector_15");
}