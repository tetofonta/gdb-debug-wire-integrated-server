#![no_std]
#![no_main]

mod lufa;
mod panic;
mod usb;

use atmega_hal::{Peripherals, Wdt};
use crate::lufa::bindings::{get_state, LUFA_Endpoint_ClearIN, LUFA_Endpoint_ClearOUT, LUFA_Endpoint_Full, LUFA_Endpoint_IsOUTReceived, LUFA_Endpoint_Read_Stream_LE, LUFA_Endpoint_Select, LUFA_Endpoint_WaitUntilReady, LUFA_Endpoint_Write_Stream_LE};
use crate::lufa::wrapper::get_line_encoding;
use crate::usb::constants::DEVICE_STATE_CONFIGURED;

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
        unsafe{
            cdc_task();
            //lufa::bindings::CDC_Task();
            lufa::bindings::LUFA_USB_Task();
        }
    }
}

unsafe fn cdc_task() -> (){
    let line_encoding = get_line_encoding();

    if get_state() != DEVICE_STATE_CONFIGURED || line_encoding.BaudRateBPS == 0{
        return
    }
    LUFA_Endpoint_Select(0x00 | 4); //rx ep
    if LUFA_Endpoint_IsOUTReceived(){
        let mut read: [u8; 50] = [0; 50];
        LUFA_Endpoint_Read_Stream_LE(read.as_mut_ptr(), 50);
        LUFA_Endpoint_ClearOUT();

        LUFA_Endpoint_Select(0x80 | 3); //tx ep
        LUFA_Endpoint_Write_Stream_LE(read.as_ptr(), read.len() as u16);
        let is_full = LUFA_Endpoint_Full();
        LUFA_Endpoint_ClearIN();
        if is_full {
            LUFA_Endpoint_WaitUntilReady();
            LUFA_Endpoint_ClearIN();
        }
    }
}