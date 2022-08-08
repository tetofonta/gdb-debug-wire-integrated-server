//
// Created by stefano on 08/08/22.
//

#ifndef ARDWINO_COMMANDS_H
#define ARDWINO_COMMANDS_H

void gdb_cmd_query(char * buffer, uint16_t len);
void gdb_cmd_v(char * buffer, uint16_t len);
void gdb_cmd_read_registers(void);
void gdb_cmd_end(void);

#endif //ARDWINO_COMMANDS_H
