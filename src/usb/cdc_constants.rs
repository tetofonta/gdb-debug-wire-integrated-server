#![allow(dead_code)]

/**< CDC class-specific request to send an encapsulated command to the device. */
pub const CDC_REQ_SEND_ENCAPSULATED_COMMAND: u8 = 0x00;
/**< CDC class-specific request to retrieve an encapsulated command response from the device. */
pub const CDC_REQ_GET_ENCAPSULATED_RESPONSE: u8 = 0x01;
/**< CDC class-specific request to set the current virtual serial port configuration settings. */
pub const CDC_REQ_SET_LINE_ENCODING: u8         = 0x20;
/**< CDC class-specific request to get the current virtual serial port configuration settings. */
pub const CDC_REQ_GET_LINE_ENCODING: u8         = 0x21;
/**< CDC class-specific request to set the current virtual serial port handshake line states. */
pub const CDC_REQ_SET_CONTROL_LINE_STATE: u8     = 0x22;
/**< CDC class-specific request to send a break to the receiver via the carrier channel. */
pub const CDC_REQ_SEND_BREAK: u8               = 0x23;
