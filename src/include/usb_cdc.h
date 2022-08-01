#ifndef _USB_CDC_H_
#define _USB_CDC_H_

#include <Drivers/USB/USB.h>
#include <Descriptors.h>
#include <avr/wdt.h>
#include <avr/power.h>

uint16_t usb_cdc_read(void *dest, size_t len);
uint16_t usb_cdc_available(void);
void usb_cdc_write(void *data, size_t len);
void usb_cdc_init(void);

extern CDC_LineEncoding_t LineEncoding;

#endif