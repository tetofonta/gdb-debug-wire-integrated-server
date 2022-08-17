//
// Created by stefano on 11/08/22.
//
#include <gdb/gdb.h>
#include <gdb/commands.h>
#include <gdb/utils.h>
#include <dw/debug_wire.h>
#include <dw/debug_wire_ll.h>
#include <string.h>
#include "usb/usb_cdc.h"

bool is_flash_write_in_progress = 0;
static uint16_t remaining = 0;
static uint16_t writing_address = 0;
static uint16_t writing_page_address = 0;
static uint16_t word = 0;
static uint8_t step = 0;

void finalize_flash_write(void){

//    uint16_t fill = 0xFFFF;
//    while( remaining )
//        remaining = dw_ll_flash_write_populate_buffer(&fill, 1, remaining);

    _delay_ms(10);
    dw_ll_flash_clear_page(writing_page_address << 1);
    _delay_ms(10);
    dw_ll_flash_write_execute();
    dw_env_close(DW_ENV_FLASH_WRITE);
    for (uint8_t i = 0; i < debug_wire_g.swbrkpt_n; ++i) {
        if(debug_wire_g.swbrkpt[i].address / debug_wire_g.device.flash_page_end == writing_address) {
            debug_wire_g.swbrkpt[i].stored = 0;
        }
    }
    writing_address = 0;
    is_flash_write_in_progress = 0;
    step = 0;
}

/**
 * limitazioni:
 * le scritture devono essere eseguite dall'inizio della pagina alla fine.
 * il buffer viene riempito con la prima scrittura e scritto al raggiungimento della fine di pagina;
 * @param data_buffer
 * @param data_length
 * @param word_address
 */
static void write_flash(char * data_buffer, uint16_t len, char * real_buffer, uint16_t real_buf_len, uint16_t data_length, uint16_t word_address){
    if(!is_flash_write_in_progress){
        if(!(word_address % debug_wire_g.device.flash_page_end)){
            dw_env_open(DW_ENV_FLASH_WRITE);
            writing_address = word_address;
            writing_page_address = word_address;
            remaining = dw_ll_flash_write_page_begin(word_address << 1);
            is_flash_write_in_progress = 1;
        } else {
            gdb_send_PSTR(PSTR("$E05#aa"), 7);
            return;
        }
    }
    uint8_t byte;
    while (data_length && remaining > 0){

        if(len < 2){
            memcpy(real_buffer, data_buffer, len);
            len = usb_cdc_read(real_buffer + len, real_buf_len - len) + len;
            data_buffer = real_buffer;
        }

        byte = (hex2nib(*data_buffer) << 4) | (hex2nib(*(data_buffer + 1)));
        len -= 2;
        data_buffer += 2;
        data_length -= 1;

        if(!(step++ & 1)) word = byte;
        else {
            word |= byte << 8;
            word_address++;
            remaining = dw_ll_flash_write_populate_buffer(&word, 1, remaining);

            if(!remaining) {
                finalize_flash_write();
                write_flash(data_buffer, len, real_buffer, real_buf_len, data_length, word_address);
                return;
            }
        }
    }
}

static void write_sram(char * data_buffer, uint16_t len, char * real_buffer, uint16_t real_buf_len, uint16_t data_length, uint16_t address){
    dw_env_open(DW_ENV_SRAM_RW);

    uint8_t byte;
    while (data_length){
        if(len < 2){
            memcpy(real_buffer, data_buffer, len);
            len = usb_cdc_read(real_buffer + len, real_buf_len - len) + len;
            data_buffer = real_buffer;
        }

        byte = (hex2nib(*data_buffer) << 4) | (hex2nib(*(data_buffer + 1)));
        len -= 2;
        data_buffer += 2;
        data_length -= 1;
        dw_ll_sram_write(address++, 1, &byte); //todo optimize
    }
    dw_env_close(DW_ENV_SRAM_RW);
}


static void write_eeprom(char * data_buffer, uint16_t len, char * real_buffer, uint16_t real_buf_len, uint16_t data_length, uint16_t address){
    dw_env_open(DW_ENV_EEPROM_RW);

    uint8_t byte;
    while (data_length){
        if(len < 2){
            memcpy(real_buffer, data_buffer, len);
            len = usb_cdc_read(real_buffer + len, real_buf_len - len) + len;
            data_buffer = real_buffer;
        }

        byte = (hex2nib(*data_buffer) << 4) | (hex2nib(*(data_buffer + 1)));
        len -= 2;
        data_buffer += 2;
        data_length -= 1;
        dw_ll_eeprom_write_byte(address++, byte);
    }
    dw_env_close(DW_ENV_EEPROM_RW);
}

void gdb_cmd_write_memory(char * buffer, uint16_t len){
    char * buf = buffer;
    uint16_t buf_len = len;

    char format = *buffer++; len--; //M<addr>,<len>:<data>#<checksum>
    uint64_t address = 0;
    uint64_t length = 0;
    buffer = parse_hex_until(',', buffer, &len, &address) + 1; len--;
    buffer = parse_hex_until(':', buffer, &len, &length) + 1; len--;

    if(format == 'M'){
        if(address < 0x800000){
            write_flash(buffer, len, buf, buf_len, length, address >> 1);
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
        } else if (address < 0x810000) {
            write_sram(buffer, len, buf, buf_len, length, address);
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
        } else {
            write_eeprom(buffer, len, buf, buf_len, length, address);
            gdb_send_PSTR(PSTR("$OK#9a"), 6);
        } //todo checks on memory size. return e01
    } else {
        gdb_send_empty(); //binary not yet implemented
    }
}