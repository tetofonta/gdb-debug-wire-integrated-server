#include <string.h>
#include <Drivers/USB/USB.h>
#include "Descriptors.h"

static CDC_LineEncoding_t LineEncoding = {
        .BaudRateBPS = 0,
        .CharFormat  = CDC_LINEENCODING_OneStopBit,
        .ParityType  = CDC_PARITY_None,
        .DataBits    = 8
};

void LUFA_USB_Init(void){
    USB_Init();
}

void LUFA_USB_Task(void){
    USB_USBTask();
}

void LUFA_set_line_encoding(CDC_LineEncoding_t encoding_data){
    LineEncoding = encoding_data;
}

CDC_LineEncoding_t * LUFA_get_line_encoding(void){
    return &LineEncoding;
}

USB_Request_Header_t * LUFA_get_last_request_header(void){
    return &USB_ControlRequest;
}

bool LUFA_Endpoint_ConfigureEndpoint(const uint8_t address, const uint8_t type, const uint16_t size, const uint8_t banks){
    return Endpoint_ConfigureEndpoint(address, type, size, banks);
}

void LUFA_Endpoint_ClearSETUP(void){
    Endpoint_ClearSETUP();
}

void LUFA_Endpoint_ClearStatusStage(void){
    Endpoint_ClearStatusStage();
}

void LUFA_Endpoint_ClearOUT(void){
    Endpoint_ClearOUT();
}

void LUFA_Endpoint_ClearIN(void){
    Endpoint_ClearIN();
}

void LUFA_Endpoint_Write_Control_Stream_LE(const uint8_t * data, uint16_t size){
    Endpoint_Write_Control_Stream_LE(data, size);
}

void LUFA_Endpoint_Read_Control_Stream_LE(uint8_t * data_dest, uint16_t len){
    Endpoint_Read_Control_Stream_LE(data_dest, len);
}

void LUFA_EP_write_line_encoding(void){
    Endpoint_Write_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
}

void LUFA_EP_read_line_encoding(void){
    Endpoint_Read_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
}

void LUFA_Endpoint_Select(uint8_t ep){
    Endpoint_SelectEndpoint(ep);
}

bool LUFA_Endpoint_IsOUTReceived(void){
    return Endpoint_IsOUTReceived();
}

void LUFA_Endpoint_Write_Stream_LE(const uint8_t * data, uint16_t len){
    Endpoint_Write_Stream_LE(data, len, NULL);
}

void LUFA_Endpoint_Read_Stream_LE(uint8_t * data, uint16_t len){
    Endpoint_Read_Stream_LE(data, len, NULL);
}

bool LUFA_Endpoint_Full(){
    return Endpoint_BytesInEndpoint() == CDC_TXRX_EPSIZE;
}

void LUFA_Endpoint_WaitUntilReady(void){
    Endpoint_WaitUntilReady();
}

uint8_t get_state(void){
    return USB_DeviceState;
}

void set_state(uint8_t new_state){
    USB_DeviceState = new_state;
}



extern void EVENT_USB_Device_Connect(void);
extern void EVENT_USB_Device_Disconnect(void);
extern void EVENT_USB_Device_ConfigurationChanged(void);
extern void EVENT_USB_Device_ControlRequest(void);

/** Function to manage CDC data transmission and reception to and from the host. */
void CDC_Task(void) {

    char *ReportString = NULL;
    static bool ActionSent = false;

    /* Device must be connected and configured for the task to run */
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    ReportString = "tetonta\r\n";

    /* Flag management - Only allow one string to be sent per action */
    if ((ReportString != NULL) && (ActionSent == false) && LineEncoding.BaudRateBPS) {

        /* Select the Serial Tx Endpoint */
        Endpoint_SelectEndpoint(CDC_TX_EPADDR);

        /* Write the String to the Endpoint */
        LUFA_Endpoint_Write_Stream_LE(ReportString, strlen(ReportString));

        /* Remember if the packet to send completely fills the endpoint */
        bool IsFull = (Endpoint_BytesInEndpoint() == CDC_TXRX_EPSIZE);

        /* Finalize the stream transfer to send the last packet */
        Endpoint_ClearIN();

        /* If the last packet filled the endpoint, send an empty packet to release the buffer on
         * the receiver (otherwise all data will be cached until a non-full packet is received) */
        if (IsFull) {
            /* Wait until the endpoint is ready for another packet */
            Endpoint_WaitUntilReady();

            /* Send an empty packet to ensure that the host does not buffer data sent to it */
            Endpoint_ClearIN();
        }
    }

    /* Select the Serial Rx Endpoint */
    Endpoint_SelectEndpoint(CDC_RX_EPADDR);

    /* Throw away any received data from the host */
    if (Endpoint_IsOUTReceived()){
        PORTD |= (1 << 5);
        while(1);
        Endpoint_ClearOUT();
    }
}

