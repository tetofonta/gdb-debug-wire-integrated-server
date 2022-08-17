
#include <avr/io.h>
#include <avr/interrupt.h>

#include <usb/usb_cdc.h>
#include <dw/open_drain.h>
#include <panic.h>
#include <user_button.h>
#include "gdb/gdb.h"
#include "dw/debug_wire.h"
#include "leds.h"
#include "stk500/stk500.h"
#include "dw/debug_wire_ll.h"

static user_button_state_t rst_button;
uint8_t mode = 0;

int main(void) {
    usr_btn_setup();
    usr_btn_init(&rst_button, &PINB, 6);

    GDB_LED_INIT();
    DW_LED_INIT();
    GDB_LED_ON();
    DW_LED_ON();

    OD_HIGH(D, 7);

    usb_cdc_init();
    sei();
    GDB_LED_OFF();
    DW_LED_OFF();

#ifdef ISP_PROGRAMMER
    stk500_init();
#else
    gdb_init();
#endif

    for (;;) {
        usr_btn_task(&rst_button);
        if (USB_DeviceState == DEVICE_STATE_Configured)
#ifdef ISP_PROGRAMMER
            stk500_task();
#else
            gdb_task();
#endif
        USB_USBTask();
    }
}

void usr_btn_event(user_button_state_t *btn) {
    if (USB_DeviceState != DEVICE_STATE_Configured || LineEncoding.BaudRateBPS != 1200) {
        debug_wire_device_reset();
        debug_wire_resume(DW_GO_CNTX_CONTINUE, false);
    }
}