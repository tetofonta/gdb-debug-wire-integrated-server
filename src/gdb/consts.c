//
// Created by stefano on 21/08/22.
//
#include <gdb/pstr.h>

const char GDB_ERR_01[] PROGMEM = "$E01#a6";
const char GDB_ERR_03[] PROGMEM = "$E03#a8";
const char GDB_ERR_05[] PROGMEM = "$E05#aa";
const char GDB_REP_OK[] PROGMEM = "$OK#9a";

const char GDB_PKT_FILTER_SUPPORTED[] PROGMEM = "Supported";
const char GDB_PKT_FILTER_QFTHREADINFO[] PROGMEM = "fThr";
const char GDB_PKT_FILTER_QSTHREADINFO[] PROGMEM = "sThr";
const char GDB_PKT_FILTER_QRCMD[] PROGMEM = "Rcmd,";
const char GDB_PKT_FILTER_MONITOR_RESET[] PROGMEM = "7265";
const char GDB_PKT_FILTER_MONITOR_RTT[] PROGMEM = "7274";
const char GDB_PKT_FILTER_MONITOR_DISABLE[] PROGMEM = "64";
const char GDB_PKT_FILTER_MONITOR_TIMERS[] PROGMEM = "74";
const char GDB_PKT_FILTER_MONITOR_SIGNATURE[] PROGMEM = "73";
const char GDB_PKT_FILTER_MONITOR_FREQUENCY[] PROGMEM = "66";
const char GDB_PKT_FILTER_MONITOR_VKILL[] PROGMEM = "Kill";

const char GDB_PKT_ANSW_SUPPORTED[] PROGMEM = "$PacketSize=3c;swbreak+#1b";
const char GDB_PKT_ANSW_QC[] PROGMEM = "$QC01#f5";
const char GDB_PKT_ANSW_THREADINFO[] PROGMEM = "$m01#ce";
const char GDB_PKT_ANSW_EOL[] PROGMEM = "$l#6c";

const char GDB_PKT_ANSW_ACK[] PROGMEM = "+";
const char GDB_PKT_ANSW_NACK[] PROGMEM = "-";
const char GDB_PKT_ANSW_EMPTY[] PROGMEM = "$#00";
const char GDB_PKT_ANSW_LF[] PROGMEM = "0a";
const char GDB_PKT_ANSW_RTT[] PROGMEM = "O7274743a20";