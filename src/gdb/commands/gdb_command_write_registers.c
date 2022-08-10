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

    while(written < 38){
        if(len < 2){
            usb_cdc_write(buf, buf_len);
            usb_cdc_write(&len, 2);
            //memcpy(buf, buffer, len); //copy what's left to the beginning
            len = usb_cdc_read(buf, buf_len);// + len; //continue reading and store after what has been copied
            usb_cdc_write(buf, len);
            buffer = buf;
        }

        if(!(byte++ & 1)) {
            val = hex2nib(*buffer++) << 4;
            len--;
        }else {
            val |= hex2nib(*buffer++);
            len--;

            if(written < 32) dw_ll_register_write(written, val);
            else if (written == 33) {
                dw_env_close(DW_ENV_REG_IO);
                dw_env_open(DW_ENV_SRAM_RW);
                dw_ll_sram_write(0x3f, 1, &val);
            }
            else if (written == 34) dw_ll_sram_write(0x3d, 1, &val);
            else if (written == 35) dw_ll_sram_write(0x3e, 1, &val);
            else if (written == 36) aux = val >> 1;
            else if (written == 37) {
                aux |= val << 7;
                aux = BE(aux);
                cur_state.pc = aux;
            }
            written ++;
        }
    }
    dw_env_close(DW_ENV_SRAM_RW);
    gdb_send_PSTR(PSTR("$OK#9a"), 6);
}

//G fa 08 97 17 07 fd fa f1 1d d3 fe d7 2c 90 b0 b9 7f a7 37 83 e2 5a fa db ce c8 9d 1c 72 9a 6 b 1f 00 00 00 00 00 00 00#c4
//  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 3 0 31 sr sp sp pc pc pc pc