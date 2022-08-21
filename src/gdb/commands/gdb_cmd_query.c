//
// Created by stefano on 08/08/22.
//
#include <gdb/gdb.h>
#include <gdb/utils.h>
#include <gdb/pstr.h>
#include <dw/debug_wire.h>
#include <avr/pgmspace.h>
#include "leds.h"
#include "dw/debug_wire_ll.h"
#include "gdb/rtt.h"
#include "panic.h"

void gdb_cmd_query(char *buffer, uint16_t len) {
    if (!memcmp_P(buffer, GDB_PKT_FILTER_SUPPORTED, 9)) {
        gdb_send_PSTR(GDB_PKT_ANSW_SUPPORTED, 26);

        if (gdb_state_g.state == GDB_STATE_SIGHUP) {
            return;
        }

        //gdb is connected!
        GDB_LED_ON();
        gdb_state_g.state = GDB_STATE_IDLE;
        debug_wire_device_reset();
        dw_env_open(DW_ENV_SRAM_RW);
        gdb_rtt_enable = 0;
        rtt_set_state(0);
        dw_env_close(DW_ENV_SRAM_RW);
        gdb_state_g.state = GDB_STATE_SIGTRAP;
    }
    else if (buffer[1] == 'C')
        gdb_send_PSTR(GDB_PKT_ANSW_QC, 8);
    else if (!memcmp_P(buffer, GDB_PKT_FILTER_QFTHREADINFO, 4)) {
        gdb_send_PSTR(GDB_PKT_ANSW_THREADINFO, 7); //qfThreadinfo
    } else if (!memcmp_P(buffer, GDB_PKT_FILTER_QSTHREADINFO, 4)) gdb_send_PSTR(GDB_PKT_ANSW_EOL, 5); //qsThreadInfo
    else if (!memcmp_P(buffer, GDB_PKT_FILTER_QRCMD, 5)) {
        //monitor command
        if (!memcmp_P(buffer + 5, GDB_PKT_FILTER_MONITOR_RESET, 4)) {
            debug_wire_device_reset();
            gdb_send_PSTR(GDB_REP_OK, 6);
        } //re - set
        else if (!memcmp_P(buffer + 5, GDB_PKT_FILTER_MONITOR_RTT, 4)) {
            dw_env_open(DW_ENV_SRAM_RW);
            if (!memcmp_P(buffer + 13, GDB_PKT_FILTER_MONITOR_DISABLE, 2)) { //rtt ]d - isable
                rtt_set_state(0);
            } else {
                rtt_set_state(2);
            }
            dw_env_close(DW_ENV_SRAM_RW);
            gdb_send_PSTR(GDB_REP_OK, 6);
        } //rt - t (d - isable/enable)
        else if (!memcmp_P(buffer + 5, GDB_PKT_FILTER_MONITOR_DISABLE, 2)) {
            MUST_SUCCEED(debug_wire_halt(), 1);
            dw_cmd(DW_CMD_DISABLE);
            gdb_send_PSTR(GDB_REP_OK, 6);
        } // d - isable
        else if (!memcmp_P(buffer + 5, GDB_PKT_FILTER_MONITOR_TIMERS, 2)) {
            if (!memcmp_P(buffer + 5 + 14, GDB_PKT_FILTER_MONITOR_DISABLE, 2)) debug_wire_g.run_timers = 0;
            else debug_wire_g.run_timers = 1;
            gdb_send_PSTR(GDB_REP_OK, 6);
        } //t - imers (d - isable/ enable)
        else if (!memcmp_P(buffer + 5, GDB_PKT_FILTER_MONITOR_SIGNATURE, 2)) {
            uint32_t signature = byte2hex(debug_wire_g.device.signature & 0xFF) |
                                 (uint32_t) byte2hex(debug_wire_g.device.signature >> 8) << 16;
            gdb_message((const char *) &signature, PSTR("O"), 4, 1);
            gdb_send_PSTR(GDB_REP_OK, 6);
        } //s - ignature (d - isable/ enable)
        else if (!memcmp_P(buffer + 5, GDB_PKT_FILTER_MONITOR_FREQUENCY, 2)) { //qRcmd,(+5)6672657175656e637920(+25)31663430
            if(len < 34) return gdb_send_PSTR(GDB_ERR_01, 8);

            char f[4] = {
                    hex2nib(*(buffer + 5 + 20)) << 4 | hex2nib(*(buffer + 5 + 21)),
                    hex2nib(*(buffer + 5 + 22)) << 4 | hex2nib(*(buffer + 5 + 23)),
                    hex2nib(*(buffer + 5 + 24)) << 4 | hex2nib(*(buffer + 5 + 25)),
                    hex2nib(*(buffer + 5 + 26)) << 4 | hex2nib(*(buffer + 5 + 27)),
                         };

            uint16_t freq = (uint16_t) hex2nib(f[0]) << 12 | (uint16_t) hex2nib(f[1]) << 8 | hex2nib(f[2]) << 4 | hex2nib(f[3]);
            gdb_deinit();
            gdb_init(freq);
            gdb_send_PSTR(GDB_REP_OK, 6);
        } //f - requency [int]
        else
            gdb_send_empty();

    } else gdb_send_empty();
}