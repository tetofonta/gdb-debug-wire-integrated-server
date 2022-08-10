#ifndef _USB_CDC_H_
#define _USB_CDC_H_

#include "Drivers/USB/USB.h"
#include "Descriptors.h"
#include <avr/wdt.h>
#include <avr/power.h>

#define USB_CDC_BUFFER_SIZE 64
#define USB_CDC_BUFFER_WORDS 32

uint16_t usb_cdc_read(void *dest, size_t len);
uint16_t usb_cdc_available(void);
void usb_cdc_write(const void *data, size_t len);
void usb_cdc_write_PSTR(const void * data, size_t len);
void usb_cdc_init(void);
uint8_t usb_cdc_read_byte(void);
void usb_cdc_discard(void);

extern CDC_LineEncoding_t LineEncoding;

extern union cdc_buffer_u{
    uint8_t as_byte_buffer[USB_CDC_BUFFER_SIZE];
    uint16_t as_word_buffer[USB_CDC_BUFFER_WORDS];
} cdc_buffer;

#endif