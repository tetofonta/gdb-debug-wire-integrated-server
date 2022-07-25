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


extern void EVENT_USB_Device_Connect(void);
extern void EVENT_USB_Device_Disconnect(void);
extern void EVENT_USB_Device_ConfigurationChanged(void);

void EVENT_USB_Device_ControlRequest(void) {
    switch (USB_ControlRequest.bRequest) {
        case CDC_REQ_GetLineEncoding:
            if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
                Endpoint_ClearSETUP();

                /* Write the line coding data to the control endpoint */
                Endpoint_Write_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
                Endpoint_ClearOUT();
            }
            break;
        case CDC_REQ_SetLineEncoding:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
                if(LineEncoding.BaudRateBPS == 19200 ){
                    DDRD |= (1 << 4);
                }
                Endpoint_ClearIN();
            }
            break;
        case CDC_REQ_SetControlLineState:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
                Endpoint_ClearSETUP();
                Endpoint_ClearStatusStage();
            }
            break;
    }
}

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
        Endpoint_Write_Stream_LE(ReportString, strlen(ReportString), NULL);

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
    if (Endpoint_IsOUTReceived())
        Endpoint_ClearOUT();
}

