use crate::usb::lufa::wrapper::{endpoint_configure, get_last_request_header, get_line_encoding};
use crate::usb::cdc_constants;
use atmega_hal::{Peripherals, pins};
use crate::usb::lufa::bindings::{LUFA_Endpoint_ClearIN, LUFA_Endpoint_ClearOUT, LUFA_Endpoint_ClearSETUP, LUFA_Endpoint_ClearStatusStage, LUFA_EP_read_line_encoding, LUFA_EP_write_line_encoding, USB_Request_Header_t};

#[no_mangle]
pub extern "C" fn EVENT_USB_Device_ConfigurationChanged() {
    endpoint_configure(0x80 | 2, 3, 8, 1).unwrap();
    endpoint_configure(0x80 | 3, 2, 16, 1).unwrap();
    endpoint_configure(0x00 | 4, 2, 16, 1).unwrap();
    let mut line_encoding_struct = get_line_encoding();
    line_encoding_struct.BaudRateBPS = 0;
}

#[no_mangle]
pub unsafe extern "C" fn EVENT_USB_Device_Connect(){
    let ic = Peripherals::steal();
    let pins = pins!(ic);

    let mut tx = pins.pd5.into_output();
    tx.set_low();
}

#[no_mangle]
pub unsafe extern "C" fn EVENT_USB_Device_Disconnect(){
    let ic = Peripherals::steal();
    let pins = pins!(ic);

    let mut tx = pins.pd5.into_output();
    tx.set_high();
}

unsafe fn cdc_get_line_encoding(header: &USB_Request_Header_t){
   if header.bmRequestType == (1 << 7) | (1 << 5) | 1 {
       LUFA_Endpoint_ClearSETUP();
       LUFA_EP_write_line_encoding();
       LUFA_Endpoint_ClearOUT();
   }
}

unsafe fn cdc_set_line_encoding(header: &USB_Request_Header_t){
    if header.bmRequestType == (0 << 7) | (1 << 5) | 1 {
        LUFA_Endpoint_ClearSETUP();
        LUFA_EP_read_line_encoding();
        LUFA_Endpoint_ClearIN();
    }
}

unsafe fn set_ctrl_state_lines(header: &USB_Request_Header_t){
    if header.bmRequestType == (0 << 7) | (1 << 5) | 1 {
        LUFA_Endpoint_ClearSETUP();
        LUFA_Endpoint_ClearStatusStage();
    }
}

#[no_mangle]
pub unsafe extern "C" fn EVENT_USB_Device_ControlRequest(){
    let header = get_last_request_header();
    match header.bRequest{
        cdc_constants::CDC_REQ_GET_LINE_ENCODING => cdc_get_line_encoding(&header),
        cdc_constants::CDC_REQ_SET_LINE_ENCODING => cdc_set_line_encoding(&header),
        cdc_constants::CDC_REQ_SET_CONTROL_LINE_STATE => set_ctrl_state_lines(&header),
        _ => return
    }
}