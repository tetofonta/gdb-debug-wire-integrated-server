use crate::lufa::bindings::LUFA_Endpoint_ConfigureEndpoint;

pub unsafe fn Endpoint_ConfigureEndpoint(address: u8, type_: u8, size: u16, banks: u8) -> Result<(), ()> {
    if LUFA_Endpoint_ConfigureEndpoint(address, type_, size, banks) {
        return Ok(())
    }
    Err(())
}