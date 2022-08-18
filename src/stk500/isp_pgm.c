//
// Created by stefano on 13/08/22.
//

#include <util/delay.h>
#include "isp/isp.h"
#include "usb/usb_cdc.h"
#include "gdb/gdb.h"

void isp_task(void){
    if (Endpoint_IsOUTReceived()) {
        uint8_t cmd = usb_cdc_read_byte();
        uint8_t answ = spi_transfer(cmd);
        usb_cdc_write(&answ, 1);
        Endpoint_SelectEndpoint(CDC_RX_EPADDR);
        if(!Endpoint_IsReadWriteAllowed()){
            Endpoint_ClearOUT();
        }
    }
}

