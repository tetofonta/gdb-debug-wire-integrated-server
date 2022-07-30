use core::cmp::min;
use embedded_hal::serial::{Read, Write};
use crate::{DEVICE_STATE_CONFIGURED, get_line_encoding, get_state, LUFA_Endpoint_ClearIN, LUFA_Endpoint_ClearOUT, LUFA_Endpoint_Full, LUFA_Endpoint_IsOUTReceived, LUFA_Endpoint_Read_Stream_LE, LUFA_Endpoint_Select, LUFA_Endpoint_WaitUntilReady, LUFA_Endpoint_Write_Stream_LE};
use crate::lufa::bindings::{CDC_LineEncoding_t, LUFA_Endpoint_BytesInEndpoints, LUFA_USB_Init, LUFA_USB_Task, USB_Request_Header_t};
use atmega_hal::{Peripherals, pins, Wdt};
use crate::lufa::wrapper::{get_last_request_header_ptr, get_line_encoding_ptr};

#[derive(Copy, Clone)]
pub struct USBSerial{
    line_encoding: *mut CDC_LineEncoding_t,
    last_usb_data: *mut USB_Request_Header_t
}

impl USBSerial{
    pub unsafe fn last_usb_data(&self) -> &USB_Request_Header_t{
        & ( * self.last_usb_data )
    }

    pub unsafe fn line_encoding(&self) -> &CDC_LineEncoding_t{
        & ( * self.line_encoding )
    }

    pub fn take() -> &'static Option<Self>{
        let ic = Peripherals::take().unwrap();
        let mut wdt = Wdt::new(ic.WDT, &ic.CPU.mcusr);

        ic.CPU.mcusr.modify(|_, w| w.wdrf().clear_bit());
        wdt.stop();

        unsafe{
            LUFA_USB_Init();
            avr_device::interrupt::enable();

            if USB_SERIAL.is_none(){
                USB_SERIAL = Some(USBSerial {
                    line_encoding: get_line_encoding_ptr(),
                    last_usb_data: get_last_request_header_ptr()
                })
            }
            return &USB_SERIAL
        }
    }

    pub unsafe fn steal() -> &'static Option<Self>{
        &USB_SERIAL
    }

    pub fn is_ready(&self) -> bool{
        unsafe{
            return get_state() == DEVICE_STATE_CONFIGURED && self.line_encoding().BaudRateBPS != 0
        }
    }

    pub fn usb_task(&self){
        unsafe{ LUFA_USB_Task(); }
    }

    pub fn available(&self) -> u16{
        unsafe{
            LUFA_Endpoint_Select(0x00 | 4);
            return LUFA_Endpoint_BytesInEndpoints();
        }
    }

    pub fn read<'a>(&self, buffer: &'a mut [u8]) -> Option<&'a mut [u8]>{
        unsafe{
            LUFA_Endpoint_Select(0x00 | 4);
            if LUFA_Endpoint_IsOUTReceived() {
                let read = LUFA_Endpoint_Read_Stream_LE(buffer.as_mut_ptr(), buffer.len() as u16);
                LUFA_Endpoint_ClearOUT();
                return Some(&mut buffer[0..read as usize]);
            }
            return None;
        }
    }

    pub fn write(&self, data: &[u8]){
        unsafe{
            LUFA_Endpoint_Select(0x80 | 3); //tx ep
            LUFA_Endpoint_Write_Stream_LE(data.as_ptr(), data.len() as u16);
            let is_full = LUFA_Endpoint_Full();
            LUFA_Endpoint_ClearIN();
            LUFA_Endpoint_WaitUntilReady();
            if is_full {
                LUFA_Endpoint_ClearIN();
            }
        }
    }
}

static mut USB_SERIAL: Option<USBSerial> = None;
