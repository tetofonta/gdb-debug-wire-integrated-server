#![no_std]
#![no_main]
#![feature(abi_avr_interrupt)]

mod panic;
mod usb;
mod debug_wire;

use atmega_hal::{pins, Wdt, Peripherals};
use usb::lufa;
use usb::lufa::bindings::{get_state, LUFA_Endpoint_ClearIN, LUFA_Endpoint_ClearOUT, LUFA_Endpoint_Full, LUFA_Endpoint_IsOUTReceived, LUFA_Endpoint_Read_Stream_LE, LUFA_Endpoint_Select, LUFA_Endpoint_WaitUntilReady, LUFA_Endpoint_Write_Stream_LE};
use usb::lufa::wrapper::get_line_encoding;
use crate::debug_wire::DebugWire;
use crate::usb::cdc::USBSerial;
use crate::usb::constants::DEVICE_STATE_CONFIGURED;

#[no_mangle]
pub extern fn main() -> ! {
    let serial = USBSerial::take().unwrap();
    let mut dw = DebugWire::take(8_000_000).unwrap();
    dw.set_divisor(64);

    loop {
        cdc_task(&serial, &mut dw);
        serial.usb_task();
    }
}

fn cdc_task(serial: &USBSerial, dw: &mut DebugWire) -> () {
    if !serial.is_ready() || serial.available() == 0 {
        return;
    }

    let mut read_buf: [u8; 5] = [0; 5];
    let read = serial.read(&mut read_buf).unwrap();

    serial.write(&[dw.send_byte(read[0]); 1]);
}