use core::panic::PanicInfo;
use atmega_hal::{clock, Peripherals, pins};
use atmega_hal::delay::Delay;

use embedded_hal::prelude::_embedded_hal_blocking_delay_DelayMs;

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