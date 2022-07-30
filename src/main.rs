#![no_std]
#![no_main]

mod panic;
mod usb;

use atmega_hal::{pins, Wdt};
use usb::lufa;
use usb::lufa::bindings::{get_state, LUFA_Endpoint_ClearIN, LUFA_Endpoint_ClearOUT, LUFA_Endpoint_Full, LUFA_Endpoint_IsOUTReceived, LUFA_Endpoint_Read_Stream_LE, LUFA_Endpoint_Select, LUFA_Endpoint_WaitUntilReady, LUFA_Endpoint_Write_Stream_LE};
use usb::lufa::wrapper::get_line_encoding;
use crate::usb::cdc::USBSerial;
use crate::usb::constants::DEVICE_STATE_CONFIGURED;

#[no_mangle]
pub extern fn main() -> ! {
    let serial = USBSerial::take().unwrap();

    loop {
        cdc_task(&serial);
        serial.usb_task();
    }
}

fn cdc_task(serial: &USBSerial) -> () {
    if !serial.is_ready() || serial.available() == 0 {
        return;
    }

    let mut read_buf: [u8; 50] = [0; 50];

    let read = serial.read(&mut read_buf).unwrap();
    read.reverse();
    serial.write(&read);
    serial.write(&(read.len().to_be_bytes()));
}