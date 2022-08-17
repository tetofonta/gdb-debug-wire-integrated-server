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

/**
 * Contains the last value to be transmitted or the received bits before stop bit occurs
 */
volatile register uint8_t uart_data asm("r2");
/**
 * Contains the flags used in the timer interrupt and provides fast access.
 * - nextval: next bit to be transmitted on the bus
 * - wrt: if the interrupt to occur is a transmission or reception duty
 * - cnt[3:0]: current operation step.
 *      those steps are:
 *          0:
 *              rx: idle
 *              tx: start bit
 *          1 - 8:
 *              rx: receiving bit (i-1), new data available and stored on 8
 *              tx: transmitting bit (i-1)
 *          9:
 *              rx: end of reception, stop bit verification for break or data
 *              tx: stop bit transmission
 *          10:
 *              tx: end of transmission, if tx_full init new tx.
 */
volatile register uint8_t fast_flags asm("r3"); // (nextval) (    ) (wrt) (     ) (cnt3)(cnt2)(cnt1)(cnt0)
/**
 * contains status flags.
 *  - avail: new data received and available
 *  - busy: operation running
 */
volatile uint8_t uart_flags;

/**
 * indicates if the tx buffer is full waiting for the current operation
 */
volatile uint8_t uart_tx_buffer_full;
uint8_t  uart_tx_buffer;
uint8_t  uart_rx_buffer[OD_UART_RX_BUFFER_SIZE];
/**
 * points to the next location on the buffer to be filled.
 * last available rx byte is in *(uart_rx_buffer + uart_rx_buffer_pointer - 1)
 */
uint8_t  volatile uart_rx_buffer_pointer;

/**
 * calculates the correct prescaler value for timer1 and the correct value for OCR1A
 * for CTC timer operations.
 * @param baud_rate wanted baudrate for the operations.
 * @return the required prescaler value or zero (timer stopped) if no prescaler is available
 */
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

/**
 * initializes timer, flags and pin irq for serial operations
 * @param baud_rate wanted baudrate
 */
void od_uart_init(uint32_t baud_rate){
    //setup counter and irq
    TIMER_IRQ_DISABLE();
    FE_IRQ_DISABLE();

    TCCR1A = (0 << COM1A0) | (0 << COM1B0) | (0 << WGM10); //no output compare
    TCCR1B = (1 << WGM12) | (_od_uart_calc_prescaler(baud_rate) << CS10); //ctc mode and prescaler. starts timer
    EICRB |= (0 << ISC70); //irq on low level. could not use falling edge irq because of slow rise rate of the line causing irq firing.

    fast_flags = 0;
    uart_flags = 0;
    uart_rx_buffer_pointer = 0;

    TCNT1 = 0;
    TIMER_IRQ_CLEAR();
    FE_IRQ_CLEAR();
    FE_IRQ_ENABLE(); //clear irqs and enable rx
}

void od_uart_deinit(void){
    TIMER_IRQ_DISABLE();
    FE_IRQ_DISABLE();
    TCCR1B = (1 << WGM12); //stop the timer
    OD_HIGH(D, 7);
}

/**
 * @return returns the flags.
 */
uint8_t od_uart_status(void){
    return (fast_flags | uart_flags) & 0xF0;
}

/**
 * places a byte in the transmission buffer and starts a new transmission if not already in progress.
 * Automatially waits for buffer full and rx completion before starting.
 * @param data
 */
void od_uart_tx_byte(uint8_t data){
    while(OD_UART_TX_FULL());
    uart_tx_buffer_full = 1;
    uart_tx_buffer = data;

    while(_OD_UART_BUSY && !_OD_UART_WRT);
    if(_OD_UART_BUSY) return; //already transmitting
    //PORTB ^= (1 << PINB7);

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

/**
 * clears rx buffer and flags
 */
void od_uart_clear(void){
    uart_rx_buffer_pointer = 0;
    uart_flags = 0;
}

/**
 * holds the line for [frames] number of frames
 * @param frames number of frames to be holding
 */
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

/**
 * holds the line high
 * @param frames number of frames to wait
 */
void od_uart_blank(uint8_t frames){
    cli();
    OD_HIGH(D, 7);
    _line_clear(frames);
    FE_IRQ_CLEAR();
    sei();
}

/**
 * holds the line low (break) for two frame
 */
void od_uart_break(void){
    cli();
    OD_LOW(D, 7);
    _line_clear(2);
    OD_HIGH(D, 7);
    FE_IRQ_CLEAR();
    sei();
}

/**
 * sends a buffer of size len.
 * for each byte waits for buffer empty and adds a new byte to the trasmission.
 * it does return before transmission ends
 * @param data buffer containing data
 * @param len langht of the buffer
 */
void od_uart_send(const void *data, uint16_t len){
    while (len--){
        while(uart_tx_buffer_full);
        od_uart_tx_byte((uint8_t) *( (uint8_t*) data++));
    }
}

/**
 * waits until a specified amount of data is received
 * @param to_read specified amount of data
 */
void od_uart_wait_until(uint16_t to_read){
    while(!_OD_UART_AVAIL || uart_rx_buffer_pointer < to_read);
}

/**
 * receives a specified amount of data and stores it in a buffer.
 * @param buffer destination
 * @param expected_len specified amount of data <= len(buffer)
 */
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

/**
 * receives one byte
 * @return received byte
 */
uint8_t od_uart_recv_byte(void){
    while(uart_rx_buffer_pointer < 1);
    return *(uart_rx_buffer + uart_rx_buffer_pointer - 1);
}

/**
 * receives one byte and checks for timeout
 * @param timeout_ms: timeout (milliseconds)
 * @return received byte
 */
uint8_t od_uart_recv_byte_timeout(uint16_t * timeout_ms){
    while(uart_rx_buffer_pointer < 1){
        _delay_ms(1);
        *timeout_ms = *timeout_ms - 1;
        if(!(*timeout_ms)) return 0;
    }
    return *(uart_rx_buffer + uart_rx_buffer_pointer - 1);
}


__attribute__((optimize("-Ofast"))) ISR(TIMER1_COMPA_vect){
    cli();
//    PORTB ^= (1 << PINB7);
    register uint8_t value = PIND; //read the bus state now, we should not wait.
    if(_OD_UART_WRT){
        if ((PORTD ^ fast_flags) & (1 << PIND7) ) //if next_value is different than current bus status (set)
            OD_TOGGLE(D, 7);                        //toggle the bus
        register uint8_t v = ++fast_flags & 15;     //increment counter
        if(v > 9 ){ //if we have finished sending the stop bit
            fast_flags = 0; //clear the flags
            uart_flags &= ~(OD_UART_FLAG_BUSY_MASK);
            if(OD_UART_TX_FULL()){ //if there's still data in the buffer clear buffer and flags, then start a new transmission.
                uart_data = uart_tx_buffer;
                fast_flags |= OD_UART_FLAG_WRT_MASK;
                uart_flags |= OD_UART_FLAG_BUSY_MASK;
                uart_tx_buffer_full = 0;
            } else { //end of tx, stop timer and enable rx
                TIMER_IRQ_DISABLE();
                FE_IRQ_CLEAR();
                FE_IRQ_ENABLE();
            }
            sei();
            return;
        } else if(v == 9){
            fast_flags |= (1 << PIND7); //we need to send the stop bit now
        } else {
            fast_flags &= ~(1 << PIND7); //send current bit
            fast_flags |= uart_data << PIND7;
            uart_data >>= 1;
        }
    } else {
        register uint8_t v = ++fast_flags & 15; //increment counter
        uart_data >>= 1; //MAKE WAY FOR THE QUEEN'S GUARD
        uart_data |= value & (1 << PIND7); //store the bit
        if( v == 8) { //last bit received
            if(uart_rx_buffer_pointer == OD_UART_RX_BUFFER_SIZE) //if we are at the end of the rx buffer
                uart_rx_buffer_pointer = 0; //cycle back to 0. data will be lost =(
            uart_rx_buffer[uart_rx_buffer_pointer++] = uart_data; //store the newly received byte
        } else if (v > 8){ //if we are receiving the stop bit
            fast_flags = 0; //clear flags
            uart_flags = OD_UART_FLAG_AVAIL_MASK; //set rx avail flag
            TIMER_IRQ_DISABLE();//disable timer, nothing to do until tx or rx irq
            TIMER_IRQ_CLEAR();
//            FE_IRQ_ENABLE();
//            FE_IRQ_CLEAR();
//            sei();

            if(value & (1 << PIND7)) //if the stop bit is high
                od_uart_irq_rx(uart_data); //fire irq
            else if(!uart_data) //if data is zero and stopbit is zero, it is a break. fire break irq
                od_uart_irq_break();
            else { //else is a frame error. ignore
                uart_rx_buffer_pointer--;
                uart_flags &= ~OD_UART_FLAG_AVAIL_MASK;
            }
            FE_IRQ_ENABLE();
            FE_IRQ_CLEAR();
            sei();
        }
    }
    sei();
}

__attribute__((optimize("-Ofast"))) ISR(INT7_vect){
    cli();
    FE_IRQ_DISABLE(); //start a new rx. clear irq
    fast_flags = 0;
    uart_flags |= OD_UART_FLAG_BUSY_MASK; //set busy flag, reset flags
    TCNT1 = OCR1A >> 2; //set counter to start at next bit. make correction for timer startup and bad things.
    TIMER_IRQ_ENABLE(); //enable timer and clear irq
    TIMER_IRQ_CLEAR();
    sei();
}
