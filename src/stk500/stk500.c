//
// Created by stefano on 13/08/22.
//

#include <util/delay.h>
#include "stk500/stk500.h"
#include "usb/usb_cdc.h"
#include "gdb/gdb.h"

parameter_t param;
static uint16_t write_address;
static bool pmode = false;

void empty_reply(void) {
    if (CRC_EOP == usb_cdc_read_byte()) {
        usb_cdc_write_PSTR(STK_INSYNC, 1);
        usb_cdc_write_PSTR(STK_OK, 1);
        return;
    }
    usb_cdc_write_PSTR(STK_OK, 1);
}

void breply(uint8_t b) {
    if (CRC_EOP == usb_cdc_read_byte()) {
        usb_cdc_write_PSTR(STK_INSYNC, 1);
        usb_cdc_write(&b, 1);
        usb_cdc_write_PSTR(STK_OK, 1);
        return;
    }
    usb_cdc_write_PSTR(STK_OK, 1);
}

void get_version(uint8_t c) {
    switch (c) {
        case 0x80:
            return breply(HWVER);
        case 0x81:
            return breply(SWMAJ);
        case 0x82:
            return breply(SWMIN);
        case 0x93:
            return breply('S'); // serial programmer
        default:
            return breply(0);
    }
}

void set_parameters(uint8_t * buffer) {
    // call this after reading parameter packet into buff[]
    memcpy(&param.devicecode, buffer, 9);
//    param.devicecode = buffer[0];
//    param.revision   = buffer[1];
//    param.progtype   = buffer[2];
//    param.parmode    = buffer[3];
//    param.polling    = buffer[4];
//    param.selftimed  = buffer[5];
//    param.lockbytes  = buffer[6];
//    param.fusebytes  = buffer[7];
//    param.flashpoll  = buffer[8];
    // ignore buff[9] (= buff[8])
    // following are 16 bits (big endian)
    param.eeprompoll = beget16(&buffer[10]);
    param.pagesize   = beget16(&buffer[12]);
    param.eepromsize = beget16(&buffer[14]);

    // 32 bits flashsize (big endian)
    param.flashsize = buffer[16] * 0x01000000
                      + buffer[17] * 0x00010000
                      + buffer[18] * 0x00000100
                      + buffer[19];

    // AVR devices have active low reset, AT89Sx are active high
    param.reset_active_high = (param.devicecode >= 0xe0);
}

void start_pmode(void){
    reset_target(true);
    spi_init(); //clk/128
    enter_pgm_mode();
    pmode = true;
}

void universal(void){
    breply(spi_transaction(usb_cdc_read_byte(), usb_cdc_read_byte(), usb_cdc_read_byte(), usb_cdc_read_byte()));
}

void program_page(void){
    bool result = false;

    uint16_t length = 256 * usb_cdc_read_byte();
    length += usb_cdc_read_byte();
    char memtype = usb_cdc_read_byte();
    if(memtype == 'F'){
        isp_write_flash(write_address, length);
    }

}

void read_page(void){
    bool result = false;

    uint16_t length = 256 * usb_cdc_read_byte();
    length += usb_cdc_read_byte();

    char memtype = usb_cdc_read_byte();
    if (CRC_EOP != usb_cdc_read_byte()) {
        usb_cdc_write_PSTR(STK_NOSYNC, 1);
        return;
    }
    usb_cdc_write_PSTR(STK_INSYNC, 1);
    if (memtype == 'F')
        result = isp_read_flash(write_address, length);
    else result = false;

    if(result)
        usb_cdc_write_PSTR(STK_INSYNC, 1);
    else
        usb_cdc_write_PSTR(STK_NOSYNC, 1);
}

void read_signature(void){
    if (CRC_EOP != usb_cdc_read_byte()) {
        usb_cdc_write_PSTR(STK_NOSYNC, 1);
        return;
    }
    usb_cdc_write_PSTR(STK_INSYNC, 1);

    uint8_t sig = spi_transaction(0x30, 0x00, 0x00, 0x00);
    usb_cdc_write(&sig, 1);
    sig = spi_transaction(0x30, 0x00, 0x01, 0x00);
    usb_cdc_write(&sig, 1);
    sig = spi_transaction(0x30, 0x00, 0x02, 0x00);
    usb_cdc_write(&sig, 1);

    usb_cdc_write_PSTR(STK_OK, 1);
}

extern uint8_t mode;
void stk500_task(void){
    if (Endpoint_IsOUTReceived()) {
        uint8_t cmd = usb_cdc_read_byte();
        switch (cmd) {
            case '0': //signon
                empty_reply();
                break;
            case '1':
                if (usb_cdc_read_byte() == CRC_EOP) {
                    usb_cdc_write_PSTR(STK_INSYNC, 1);
                    usb_cdc_write_PSTR(PSTR("AVR ISP"), 7);
                    usb_cdc_write_PSTR(STK_OK, 1);
                } else {
                    usb_cdc_write_PSTR(STK_NOSYNC, 1);
                }
                break;
            case 'A':
                get_version(usb_cdc_read_byte());
                break;
            case 'B':
                usb_cdc_read(cdc_buffer.as_byte_buffer, 20);
                set_parameters(cdc_buffer.as_byte_buffer);
                empty_reply();
                break;
            case 'E':
                usb_cdc_read(cdc_buffer.as_byte_buffer, 5);
                empty_reply();
                break;
            case 'P':
                if (!pmode)
                    start_pmode();
                empty_reply();
                break;
            case 'U':
                write_address = usb_cdc_read_byte();
                write_address += 256 * usb_cdc_read_byte();
                empty_reply();
                break;

            case 0x60: //STK_PROG_FLASH
                usb_cdc_read_byte(); // low addr
                usb_cdc_read_byte(); // high addr
                empty_reply();
                break;
            case 0x61: //STK_PROG_DATA
                usb_cdc_read_byte(); // data
                empty_reply();
                break;

            case 0x64: //STK_PROG_PAGE
                program_page();
                break;

            case 0x74: //STK_READ_PAGE 't'
                read_page();
                break;

            case 'V':
                universal();
                break;
            case 'Q':
                exit_pgm_mode();
                empty_reply();
                mode = 0;
                gdb_init();
                break;

            case 0x75: //STK_READ_SIGN 'u'
                read_signature();
                break;

            case CRC_EOP:
                usb_cdc_write_PSTR(STK_NOSYNC, 1);

            default:
                if (CRC_EOP == usb_cdc_read_byte())
                    usb_cdc_write_PSTR(STK_UNKNOWN, 1);
                else
                    usb_cdc_write_PSTR(STK_NOSYNC, 1);
        }
        Endpoint_SelectEndpoint(CDC_RX_EPADDR);
        if(!Endpoint_IsReadWriteAllowed()){
            Endpoint_ClearOUT();
        }
    }
}

