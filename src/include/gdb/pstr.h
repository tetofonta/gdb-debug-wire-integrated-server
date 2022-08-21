//
// Created by stefano on 21/08/22.
//

#ifndef ARDWINO_PSTR_H
#define ARDWINO_PSTR_H

#include <avr/pgmspace.h>

extern const char GDB_ERR_01[8] PROGMEM;
extern const char GDB_ERR_03[8] PROGMEM;
extern const char GDB_ERR_05[8] PROGMEM;
extern const char GDB_REP_OK[6] PROGMEM;

extern const char GDB_PKT_FILTER_SUPPORTED[9] PROGMEM;
extern const char GDB_PKT_FILTER_QFTHREADINFO[4] PROGMEM;
extern const char GDB_PKT_FILTER_QSTHREADINFO[4] PROGMEM;
extern const char GDB_PKT_FILTER_QRCMD[5] PROGMEM;
extern const char GDB_PKT_FILTER_MONITOR_RESET[4] PROGMEM;
extern const char GDB_PKT_FILTER_MONITOR_RTT[4] PROGMEM;
extern const char GDB_PKT_FILTER_MONITOR_DISABLE[2] PROGMEM;
extern const char GDB_PKT_FILTER_MONITOR_TIMERS[2] PROGMEM;
extern const char GDB_PKT_FILTER_MONITOR_SIGNATURE[2] PROGMEM;
extern const char GDB_PKT_FILTER_MONITOR_FREQUENCY[2] PROGMEM;
extern const char GDB_PKT_FILTER_MONITOR_VKILL[4] PROGMEM;

extern const char GDB_PKT_ANSW_SUPPORTED[26] PROGMEM;
extern const char GDB_PKT_ANSW_QC[8] PROGMEM;
extern const char GDB_PKT_ANSW_THREADINFO[7] PROGMEM;
extern const char GDB_PKT_ANSW_EOL[5] PROGMEM;

extern const char GDB_PKT_ANSW_ACK[1] PROGMEM;
extern const char GDB_PKT_ANSW_NACK[1] PROGMEM;
extern const char GDB_PKT_ANSW_EMPTY[4] PROGMEM;
extern const char GDB_PKT_ANSW_LF[2] PROGMEM;
extern const char GDB_PKT_ANSW_RTT[11] PROGMEM;


#endif //ARDWINO_PSTR_H
