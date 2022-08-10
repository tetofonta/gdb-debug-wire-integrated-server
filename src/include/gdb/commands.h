//
// Created by stefano on 08/08/22.
//

#ifndef ARDWINO_COMMANDS_H
#define ARDWINO_COMMANDS_H

void gdb_cmd_query(char * buffer, uint16_t len);
void gdb_cmd_v(char * buffer, uint16_t len);
void gdb_cmd_read_registers(void);
void gdb_cmd_write_registers(char * buffer, uint16_t len);
void gdb_cmd_end(uint8_t restart, uint16_t * buffer, uint16_t len);
void gdb_cmd_read_memory(char * buffer, uint16_t len);
void gdb_cmd_breakpoint(char * buffer, uint16_t len);

#endif //ARDWINO_COMMANDS_H
