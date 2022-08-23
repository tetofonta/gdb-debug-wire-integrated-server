
#include <avr/io.h>
#include <avr/interrupt.h>

#include <usb/usb_cdc.h>
#include <dw/open_drain.h>
#include <panic.h>
#include <user_button.h>
#include "gdb/gdb.h"
#include "dw/debug_wire.h"
#include "leds.h"
#include "isp/isp.h"
#include "dw/debug_wire_ll.h"

static user_button_state_t rst_button;
static void (*task)(void) = gdb_task;

static void mode_gdb(void){
    isp_deinit();
    gdb_init(0);
    task = gdb_task;
}

static void mode_isp(void){
    gdb_deinit();
    isp_init();
    task = isp_task;
}

__attribute__((noreturn)) int main(void) {
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

    gdb_init(16000);

    for (;;) {
        usr_btn_task(&rst_button);

        if(LineEncoding.BaudRateBPS == 1200 && task == gdb_task)
            mode_isp();
        else if (LineEncoding.BaudRateBPS != 1200 && task == isp_task)
            mode_gdb();

        if (USB_DeviceState == DEVICE_STATE_Configured){
            task();
            Endpoint_SelectEndpoint(CDC_RX_EPADDR);
            if(!Endpoint_IsReadWriteAllowed()){
                Endpoint_ClearOUT();
            }
        }

        USB_USBTask();
    }
}

void usr_btn_event(user_button_state_t *btn) {
    if (USB_DeviceState != DEVICE_STATE_Configured || LineEncoding.BaudRateBPS != 1200) {
        debug_wire_device_reset();
        debug_wire_resume(DW_GO_CNTX_CONTINUE);
    }
}