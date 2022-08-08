#include "usb/usb_cdc.h"

CDC_LineEncoding_t LineEncoding = {.BaudRateBPS = 0,
        .CharFormat  = CDC_LINEENCODING_OneStopBit,
        .ParityType  = CDC_PARITY_None,
        .DataBits    = 8
};

uint16_t usb_cdc_read(void *dest, size_t len) {
    /* Select the Serial Rx Endpoint */
    Endpoint_SelectEndpoint(CDC_RX_EPADDR);
    if (Endpoint_IsOUTReceived()) {
        size_t processed = 0;
        while (Endpoint_Read_Stream_LE(dest, len, &processed) == ENDPOINT_RWSTREAM_IncompleteTransfer);
        Endpoint_ClearOUT();
        return processed;
    }
    return 0;
}

uint16_t usb_cdc_available(void) {
    Endpoint_SelectEndpoint(CDC_RX_EPADDR);
    return Endpoint_BytesInEndpoint();
}

void usb_cdc_write(const void *data, size_t len) {
    Endpoint_SelectEndpoint(CDC_TX_EPADDR);
    Endpoint_Write_Stream_LE(data, len, NULL);
    bool is_full = Endpoint_BytesInEndpoint() == CDC_TXRX_EPSIZE;
    Endpoint_ClearIN();
    if (is_full) {
        Endpoint_WaitUntilReady();
        Endpoint_ClearIN();
    }
}

void usb_cdc_write_PSTR(const void * data, size_t len) {
    Endpoint_SelectEndpoint(CDC_TX_EPADDR);
    Endpoint_Write_PStream_LE(data, len, NULL);
    bool is_full = Endpoint_BytesInEndpoint() == CDC_TXRX_EPSIZE;
    Endpoint_ClearIN();
    if (is_full) {
        Endpoint_WaitUntilReady();
        Endpoint_ClearIN();
    }
}

uint8_t usb_cdc_read_byte(void){
    Endpoint_SelectEndpoint(CDC_RX_EPADDR);
    uint8_t r = Endpoint_Read_8();
    if(!Endpoint_IsReadWriteAllowed()){
        Endpoint_ClearOUT();
        USB_USBTask();
    }
    Endpoint_WaitUntilReady();
    return r;
}

uint8_t usb_cdc_discard(void){
    Endpoint_SelectEndpoint(CDC_RX_EPADDR);
    while(Endpoint_BytesInEndpoint()) Endpoint_ClearIN();
}

void usb_cdc_init(void){
    MCUSR &= ~(1 << WDRF);
    wdt_disable();
    clock_prescale_set(clock_div_1);
    USB_Init();
}