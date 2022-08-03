#include "usb/usb_cdc.h"

void EVENT_USB_Device_Connect(void) {
    PORTD &= ~(1 << PIND5);
}
void EVENT_USB_Device_Disconnect(void) {
    PORTD |= (1 << PIND5);
}

void EVENT_USB_Device_ConfigurationChanged(void) {
    bool ConfigSuccess = true;
    ConfigSuccess &= Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPADDR, EP_TYPE_INTERRUPT, CDC_NOTIFICATION_EPSIZE, 1);
    ConfigSuccess &= Endpoint_ConfigureEndpoint(CDC_TX_EPADDR, EP_TYPE_BULK, CDC_TXRX_EPSIZE, 1);
    ConfigSuccess &= Endpoint_ConfigureEndpoint(CDC_RX_EPADDR, EP_TYPE_BULK, CDC_TXRX_EPSIZE, 1);
    LineEncoding.BaudRateBPS = 0;

}
void EVENT_USB_Device_ControlRequest(void) {
    /* Process CDC specific control requests */
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

                /* Read the line coding data in from the host into the global struct */
                Endpoint_Read_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
                Endpoint_ClearIN();
            }

            break;
        case CDC_REQ_SetControlLineState:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
                Endpoint_ClearSETUP();
                Endpoint_ClearStatusStage();

                /* NOTE: Here you can read in the line state mask from the host, to get the current state of the output handshake
                         lines. The mask is read in from the wValue parameter in USB_ControlRequest, and can be masked against the
                         CONTROL_LINE_OUT_* masks to determine the RTS and DTR line states using the following code:
                */
            }

            break;
    }
}
