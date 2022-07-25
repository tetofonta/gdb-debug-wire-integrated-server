#![no_std]
#![no_main]

mod lufa;

use core::panic::PanicInfo;
use atmega_hal::{clock, Peripherals, pins, Wdt};
use atmega_hal::delay::Delay;
use embedded_hal::prelude::_embedded_hal_blocking_delay_DelayMs;
use crate::lufa::bindings::{LUFA_get_line_encoding};
use crate::lufa::wrapper::Endpoint_ConfigureEndpoint;

#[no_mangle]
pub extern fn main() -> !{
    let ic = Peripherals::take().unwrap();
    let mut wdt = Wdt::new(ic.WDT, &ic.CPU.mcusr);

    ic.CPU.mcusr.modify(|_, w| w.wdrf().clear_bit());
    wdt.stop();

    unsafe {
        lufa::bindings::LUFA_USB_Init();
        avr_device::interrupt::enable();
    };

    loop{
        unsafe{ lufa::bindings::CDC_Task(); } //tmp
        unsafe{ lufa::bindings::LUFA_USB_Task(); }
    }
}


#[panic_handler]
unsafe fn panic_handler(_reason: &PanicInfo) -> !{
    let ic = Peripherals::steal();
    let pins = pins!(ic);
    let mut delay = Delay::<clock::MHz16>::new();
    let mut tx_pin = pins.pd5.into_output();
    loop{
        tx_pin.toggle();
        delay.delay_ms(500 as u16);
    }
}

#[no_mangle]
pub unsafe extern "C" fn EVENT_USB_Device_ConfigurationChanged() {
    Endpoint_ConfigureEndpoint(0x80 | 2, 3, 8, 1).unwrap();
    Endpoint_ConfigureEndpoint(0x80 | 3, 2, 16, 1).unwrap();
    Endpoint_ConfigureEndpoint(0x80 | 4, 2, 16, 1).unwrap();
    let mut line_encoding_struct = * LUFA_get_line_encoding();
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