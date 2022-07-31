use atmega_hal::{Peripherals, pins};
use atmega_hal::port::mode::{OpenDrain};
use atmega_hal::port::{PD7, Pin};
use avr_device;
use avr_device::atmega32u4::TC0;

#[derive(Copy, Clone)]
pub struct DebugWire {
    rx_buf: [u8; 8],
    rx_count: u8,
    pin: *mut Pin<OpenDrain, PD7>,
    timer: *const TC0,
    tgt_clk: u32,

    tx_byte: u8,
    tx_bits: u8,
}

static mut DBG_WIRE: Option<DebugWire> = None;

#[avr_device::interrupt(atmega8u2)]
unsafe fn TIMER0_COMPA() {
    DebugWire::steal().unwrap().tx_irq();
}

impl DebugWire {
    pub fn take(tgt_clk: u32) -> &'static Option<DebugWire> {
        unsafe {
            let ic = Peripherals::steal();
            let mut pin_od = pins!(ic).pd7.into_opendrain_high();

            ic.TC0.tccr0a.write(|w| {
                w
                    .wgm0().ctc()
                    .com0a().disconnected()
                    .com0b().disconnected()
            });
            ic.TC0.tccr0b.write(|w| {
                w.cs0().no_clock()
            });
            ic.TC0.timsk0.write(|w| {
                w
                    .ocie0b().clear_bit()
                    .ocie0a().clear_bit()
                    .toie0().clear_bit()
            });

            if DBG_WIRE.is_none() {
                DBG_WIRE = Some(DebugWire { rx_buf: [0; 8], rx_count: 0, pin: &mut pin_od, tgt_clk, tx_byte: 0, tx_bits: 0, timer: &ic.TC0 })
            }
            return &DBG_WIRE;
        }
    }

    pub unsafe fn steal() -> &'static Option<DebugWire> {
        return &DBG_WIRE;
    }

    pub fn set_divisor(&mut self, divisor: u32) {
        let cycles = (16000000 / self.tgt_clk * divisor) as u16;
        unsafe{
            if cycles < 256{
                self.timer().tccr0b.modify(|_, w|{w.wgm02().clear_bit().cs0().direct()});
                self.timer().ocr0a.write(|w|{w.bits(cycles as u8)})
            } else if cycles / 8 < 256{
                self.timer().tccr0b.modify(|_, w|{w.wgm02().clear_bit().cs0().prescale_8()});
                self.timer().ocr0a.write(|w|{w.bits((cycles / 8) as u8)})
            } else {
                //should not go over
                panic!();
            }
        }

    }

    fn pin(&self) -> &mut Pin<OpenDrain, PD7> {
        unsafe { return &mut (*self.pin); }
    }
    fn timer(&self) -> &TC0{
        unsafe{
            &(* self.timer)
        }
    }

    pub fn send_byte(&mut self, byte: u8) -> u8 {
        self.tx_byte = byte;
        self.tx_bits = 0;
        self.timer().tcnt0.write(|w| unsafe {w.bits(0)});
        self.timer().timsk0.modify(|_, w| {w.ocie0a().set_bit()});
        self.pin().set_low(); //start bit;
        return self.timer().ocr0a.read().bits()
    }

    pub fn tx_irq(&mut self){
        self.pin().set_high();

    }
}