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
#include "gdb/rtt.h"


void gdb_cmd_read_memory(char * buffer, uint16_t len){
    uint64_t address = 0;
    uint64_t length = 0;

    buffer = parse_hex_until(',', buffer, &len, &address) + 1;
    buffer = parse_hex_until('#', buffer, &len, &length);

    if(!debug_wire_g.halted || length > 64){
        gdb_send_PSTR(PSTR("$E01#a6"), 7);
    }

    uint16_t checksum = 0, tmp;
    if(address >= 0x810000){
        dw_env_open(DW_ENV_EEPROM_RW);
        dw_ll_eeprom_read(buffer, address & 0xFFFF, length & 0xFFFF);
        dw_env_close(DW_ENV_EEPROM_RW);
    } else if (address >= 0x800000){
        dw_env_open(DW_ENV_SRAM_RW);
        dw_ll_sram_read(address & 0xFFFF, length & 0xFFFF, buffer);
        dw_env_close(DW_ENV_SRAM_RW);
    } else {
        dw_env_open(DW_ENV_FLASH_READ);
        dw_ll_flash_read(address & 0xFFFF, length & 0xFFFF, buffer);
        dw_env_close(DW_ENV_FLASH_READ);

        for (uint8_t i = 0; i < debug_wire_g.swbrkpt_n; ++i) {
            if(!debug_wire_g.swbrkpt[i].stored) continue;
            if(debug_wire_g.swbrkpt[i].address < ((address & 0xFFFF) >> 1)) continue;
            if(debug_wire_g.swbrkpt[i].address > (((address + length - 2) & 0xFFFF) >> 1)) break;
            uint16_t offset = debug_wire_g.swbrkpt[i].address*2 - address;
            memcpy(buffer + offset, &debug_wire_g.swbrkpt[i].opcode, 2);
        }
    }

    gdb_send_begin();
    while (length--) {
        tmp = byte2hex(*buffer++);
        checksum = gdb_send_add_data(&tmp, 2, checksum);
    }
    gdb_send_finalize(checksum);
}