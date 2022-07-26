//
// Created by stefano on 08/08/22.
//
#include <gdb/gdb.h>
#include <gdb/utils.h>
#include <dw/debug_wire.h>
#include <dw/debug_wire_ll.h>
#include "panic.h"
#include "gdb/pstr.h"

void gdb_cmd_read_registers(void){
    if(!debug_wire_g.halted) return gdb_send_PSTR(GDB_ERR_05, 7);

    dw_env_open(DW_ENV_SRAM_RW);

    uint16_t tmp;
    uint8_t checksum = gdb_send_begin();
    dw_ll_registers_read(0, 31, uart_rx_buffer);
    for (int i = 0; i < 32; ++i) {
        tmp = byte2hex(uart_rx_buffer[i]);
        checksum = gdb_send_add_data((const char *) &tmp, 2, checksum);
    }

    dw_ll_sram_read(0x3d, 3, uart_rx_buffer);
    //sreg
    tmp = byte2hex(uart_rx_buffer[2] & 0xFF);
    checksum = gdb_send_add_data((const char *) &tmp, 2, checksum);

    //sp
    tmp = byte2hex(uart_rx_buffer[0] & 0xFF);
    checksum = gdb_send_add_data((const char *) &tmp, 2, checksum);
    tmp = byte2hex(uart_rx_buffer[1] & 0xFF);
    checksum = gdb_send_add_data((const char *) &tmp, 2, checksum);

    //pc
    uint16_t pc_le = (cur_state.pc) << 8 | cur_state.pc >> 8;
    tmp = byte2hex((--pc_le << 1) & 0xff);
    checksum = gdb_send_add_data((const char *) &tmp, 2, checksum);
    tmp = byte2hex(pc_le >> 7);
    checksum = gdb_send_add_data((const char *) &tmp, 2, checksum);
    tmp = ('0' + (pc_le >> 15)) | '0' << 8;
    checksum = gdb_send_add_data((const char *) &tmp, 2, checksum);
    checksum = gdb_send_add_data((const char *) &tmp, 2, checksum);

    dw_env_close(DW_ENV_SRAM_RW);
    gdb_send_finalize(checksum);
}