//
// Created by stefano on 13/08/22.
//

#ifndef ARDWINO_ISP_H
#define ARDWINO_ISP_H

#include <avr/io.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

#define STK_OK                  PSTR("\x10")
#define STK_FAILED              PSTR("\x11")
#define STK_UNKNOWN             PSTR("\x12")
#define STK_INSYNC              PSTR("\x14")
#define STK_NOSYNC              PSTR("\x15")
#define CRC_EOP                 '\x20'

#define HWVER 2
#define SWMAJ 1
#define SWMIN 18

#define beget16(addr) (*addr * 256 + *(addr+1) )
typedef struct param {
    uint8_t devicecode;
    uint8_t revision;
    uint8_t progtype;
    uint8_t parmode;
    uint8_t polling;
    uint8_t selftimed;
    uint8_t lockbytes;
    uint8_t fusebytes;
    uint8_t flashpoll;
    uint16_t eeprompoll;
    uint16_t pagesize;
    uint16_t eepromsize;
    uint32_t flashsize;
    uint8_t reset_active_high : 1;
} __attribute__((packed)) parameter_t;
extern parameter_t param;

void isp_task(void);
void isp_deinit(void);
void isp_init(void);

void spi_init(void);
void spi_deinit(void);
uint8_t spi_transfer(uint8_t byte);

#endif //ARDWINO_ISP_H
