//
// Created by stefano on 25/07/22.
//

#ifndef ARDWINO_BINDINGS_H
#define ARDWINO_BINDINGS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct{
    uint32_t BaudRateBPS;
    uint8_t  CharFormat;
    uint8_t  ParityType;
    uint8_t  DataBits;
} __attribute__((packed)) CDC_LineEncoding_t;

typedef struct
{
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) USB_Request_Header_t;

void LUFA_USB_Init(void);
void LUFA_USB_Task(void);

CDC_LineEncoding_t * LUFA_get_line_encoding(void);
void LUFA_set_line_encoding(CDC_LineEncoding_t encoding_data);

USB_Request_Header_t * LUFA_get_last_request_header(void);
bool LUFA_Endpoint_ConfigureEndpoint(uint8_t address, uint8_t type, uint16_t size, uint8_t banks);
void LUFA_Endpoint_ClearSETUP(void);
void LUFA_Endpoint_ClearStatusStage(void);
void LUFA_Endpoint_ClearOUT(void);
void LUFA_Endpoint_ClearIN(void);
void LUFA_Endpoint_Write_Control_Stream_LE(const uint8_t * data, uint16_t size);
void LUFA_Endpoint_Read_Control_Stream_LE(uint8_t * data_dest, uint16_t len);
void LUFA_EP_write_line_encoding(void);
void LUFA_EP_read_line_encoding(void);
void LUFA_Endpoint_Select(uint8_t ep);
bool LUFA_Endpoint_IsOUTReceived(void);
void LUFA_Endpoint_Write_Stream_LE(const uint8_t * data, uint16_t len);
void LUFA_Endpoint_Read_Stream_LE(uint8_t * data, uint16_t len);
bool LUFA_Endpoint_Full();
void LUFA_Endpoint_WaitUntilReady(void);
uint8_t get_state(void);
void set_state(uint8_t new_state);
void CDC_Task(void);

#endif //ARDWINO_BINDINGS_H
