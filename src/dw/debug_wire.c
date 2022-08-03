//
// Created by stefano on 03/08/22.
//
#include <dw/debug_wire.h>

void dw_full_reset(void){
    if(debug_wire_g.status == DW_STATUS_RUNNING)
        dw_cmd_halt();

    dw_cmd_reset();
    dw_cmd_multi_const(( 0x0D | DW_CMD_REG_PC ), 2, 0, 0);
    dw_set_context(DW_GO_CNTX_CONTINUE);
    dw_cmd_multi_const(( 0x0D | DW_CMD_REG_PC ), 2, 0, 0);
    dw_cmd_go();
}