//
// Created by stefano on 08/08/22.
//

#ifndef ARDWINO_GDB_H
#define ARDWINO_GDB_H

#include <avr/io.h>

#define GDB_STATE_IDLE          0 //ok
#define GDB_STATE_SIGHUP        1 //device disconnected
#define GDB_STATE_SIGINT        2 //interrupted by user
#define GDB_STATE_SIGILL        4 //illegal instruction
#define GDB_STATE_SIGTRAP       5 //stopped on a breakpoint
#define GDB_STATE_SIGABRT       6 //fatal error
#define GDB_STATE_DISCONNECTED  15

struct gdb_state {
    uint8_t state: 4;
};
extern struct gdb_state gdb_state_g;
extern uint8_t gdb_rtt_enable;

void gdb_task(void);

void gdb_init(void);
void gdb_deinit(void);

void gdb_send(const char *data, uint16_t len);

void gdb_send_PSTR(const char *data, uint16_t len);

void gdb_send_empty(void);

void gdb_send_ack(void);

void gdb_send_nack(void);

uint8_t gdb_send_begin(void);

uint8_t gdb_send_add_data(const char *data, uint16_t len, uint8_t checksum);
uint8_t gdb_send_add_data_PSTR(const char * data, uint16_t len, uint8_t checksum);
void gdb_send_finalize(uint8_t checksum);

#endif //ARDWINO_GDB_H
