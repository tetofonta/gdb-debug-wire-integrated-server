//
// Created by stefano on 08/08/22.
//
#include <gdb/gdb.h>
#include <gdb/utils.h>
#include <dw/debug_wire.h>
#include <dw/debug_wire_ll.h>
#include <string.h>
#include "panic.h"
#include "usb/usb_cdc.h"
#include "avr_isa.h"

void gdb_cmd_write_registers(char * buffer, uint16_t len){

    uint16_t buf_len = len;
    char * buf = buffer;
    buffer++; len--; //ignore G
    if(!debug_wire_g.halted) panic();

    dw_env_open(DW_ENV_REG_IO);

    uint8_t val = 0;
    uint16_t aux = 0;
    uint8_t byte = 0;
    uint8_t written = 0;

    while(written < 37){
        if(len < 2){
            memcpy(buf, buffer, len); //copy what's left to the beginning
            len = usb_cdc_read(buf + len, buf_len - len) + len; //continue reading and store after what has been copied
            buffer = buf;
        }

        if(!(byte++ & 1)) {
            val = hex2nib(*buffer++) << 4;
            len--;
        }else {
            _delay_ms(10);
            val |= hex2nib(*buffer++);
            len--;

            if(written < 32) dw_ll_register_write(written, val);
            else if (written == 32) {
                dw_env_close(DW_ENV_REG_IO);
                dw_env_open(DW_ENV_SRAM_RW);
                dw_ll_sram_write(0x3f, 1, &val);
            }
            else if (written == 33) dw_ll_sram_write(0x3d, 1, &val);
            else if (written == 34) dw_ll_sram_write(0x3e, 1, &val);
            else if (written == 35) aux = val >> 1;
            else if (written == 36) {
                aux |= val << 7;
                cur_state.pc = BE((aux+1));
                debug_wire_g.program_counter = BE(aux);
            }
            written ++;
        }
    }
    dw_env_close(DW_ENV_SRAM_RW);
    gdb_send_PSTR(PSTR("$OK#9a"), 6);
}