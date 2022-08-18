//
// Created by stefano on 13/08/22.
//
#include "isp/isp.h"
#include <util/delay.h>
#include "dw/open_drain.h"
#include "usb/usb_cdc.h"

void reset_target(bool reset) {
    if((reset && param.reset_active_high) || (!reset && !param.reset_active_high)) OD_HIGH(D, 7);
    else OD_LOW(D, 7);
}