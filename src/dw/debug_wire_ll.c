//
// Created by stefano on 31/07/22.
//

#include <avr/io.h>
#include <util/delay.h>
#include <dw/debug_wire.h>
#include <dw/devices.h>
#include <panic.h>
#include "avr_isa.h"

debug_wire_t debug_wire_g;

static volatile uint8_t waiting_break, ignore_break;
//=========================================================================================================base commands
uint8_t dw_init(uint32_t target_freq) {
    cli();
    debug_wire_g.target_frequency = target_freq;
    od_uart_clear();
    od_uart_init(target_freq >> 7);
    sei();

    if(debug_wire_g.device.signature == 0x00){
        MUST_SUCCEED(dw_cmd_halt(), 1);
        uint16_t signature = dw_cmd_get(DW_CMD_REG_SIGNATURE);

        uint16_t len = pgm_read_word(&devices.items);
        dw_device_definition_t * devs = (dw_device_definition_t *) &devices.devices;
        while(len--){
            if(pgm_read_word(&devs->signature) == signature){
                memcpy_P(&debug_wire_g.device, devs, sizeof(dw_device_definition_t));
                return 1;
            }
            devs++;
        }
        return 0;
    }
    return 1;
}

//vi sono due tipi di break:
// nel caso di halt da stato runnning: il controllore risponde dopo la fine del break e ritorna 0x55
// nel casp di halt inatteso (già halted) il controllore asserisce oltre la durata causando un interrupt startbit
// (todo solve: è perchè uso irq low invece che falling edge)
uint8_t dw_cmd_halt(void){
    od_uart_clear();
    od_uart_break();
    uint8_t ret;
    od_uart_recv(&ret, 1);
    PORTB ^= (1 << PINB7);
    if(ret != 0x55)
        od_uart_recv(&ret, 1);
    if(ret != 0x55) return 0;
    debug_wire_g.halted = 1;
    MUST_SUCCEED(dw_cmd_set_speed(DW_DIVISOR_128), 1);
    debug_wire_g.program_counter = dw_cmd_get(DW_CMD_REG_PC); //save the program counter
    return 1;
}

uint16_t dw_cmd_get_16(uint8_t cmd){
    uint16_t ret;
    od_uart_clear();
    dw_cmd(cmd);
    od_uart_recv(&ret, 2);
    return ret;
}

uint8_t dw_cmd_get_8(uint8_t cmd){
    uint8_t ret;
    od_uart_clear();
    dw_cmd(cmd);
    od_uart_recv(&ret, 1);
    return ret;
}

uint8_t dw_cmd_set_speed(uint8_t divisor){
    debug_wire_g.cur_divisor = divisor;
    return dw_cmd_get_8(divisor) == 0x55;
}

void dw_cmd_send_multiple(uint8_t command, void * data, uint8_t len){
    dw_cmd(command);
    od_uart_send(data, len);
}

void dw_cmd_send_multiple_consts(uint8_t command, uint16_t n, ...){
    va_list ptr;
    va_start(ptr, n);
    dw_cmd(command);
    while(n--)od_uart_tx_byte((uint8_t) va_arg(ptr, int));
    va_end(ptr);
}

void dw_cmd_reset(void){
    od_uart_clear();
    waiting_break = 1;
    dw_cmd(DW_CMD_RESET);
    while(waiting_break);
}

void dw_cmd_go(uint8_t is_sw_brkpt){
    debug_wire_g.halted = 0;
    dw_cmd(is_sw_brkpt ? DW_CMD_GO_FROM_IR : DW_CMD_GO);
}

//==================================================================================================================irqs
inline void od_uart_irq_rx(uint8_t data){}
inline void od_uart_irq_break(void){
    od_uart_blank(1);
    od_uart_clear();
    dw_init(debug_wire_g.target_frequency);
    od_uart_blank(8);
    od_uart_clear();
    dw_cmd_set_speed(debug_wire_g.cur_divisor);
    if(ignore_break){
        ignore_break = 0;
        waiting_break = 0;
        return;
    }
    debug_wire_g.program_counter = dw_cmd_get(DW_CMD_REG_PC); //save the program counter
    debug_wire_g.halted = 1;
    waiting_break = 0;
}

//===================================================================================================low level functions

void dw_ll_exec(uint16_t instruction, uint8_t long_instruction){
    dw_cmd_set(DW_CMD_REG_IR, &instruction);
    if(long_instruction){
        waiting_break = 1;
        ignore_break = 1;
        dw_cmd(DW_CMD_EXECUTE_LOADED_INSTR_LNG);
        while(waiting_break);
    } else {
        dw_cmd(DW_CMD_EXECUTE_LOADED_INSTR);
    }
}

static void dw_ll_reg_rw_setup(uint8_t from, uint8_t to, uint8_t target){
    uint16_t from_be = from << 8 | from >> 8;
    uint16_t to_be = to << 8 | to >> 8;

    dw_set_context(DW_GO_CNTX_MEM);
    dw_cmd_set(DW_CMD_REG_PC, &from_be);
    dw_cmd_set(DW_CMD_REG_HWBP, &to_be);
    dw_cmd_mode_set(target);
    if(target == DW_MODE_REGS_WRITE && (to - from) == 1)
        dw_cmd_start_mem_cycle_ss();
    else
        dw_cmd_start_mem_cycle();
}
uint8_t dw_ll_read_register(uint8_t reg){
    dw_ll_reg_rw_setup(reg, reg + 1, DW_MODE_REGS_READ);
    return od_uart_recv_byte();
}
void dw_ll_write_register(uint8_t reg, uint8_t data){
    dw_ll_reg_rw_setup(reg, reg + 1, DW_MODE_REGS_WRITE);
    od_uart_tx_byte(data);
}
void dw_ll_read_registers(uint8_t from, uint8_t to, void * buffer){
    dw_ll_reg_rw_setup(from, to, DW_MODE_REGS_READ);
    od_uart_recv(buffer, to - from);
}
void dw_ll_write_registers(uint8_t from, uint8_t to, const void * buffer){
    dw_ll_reg_rw_setup(from, to, DW_MODE_REGS_WRITE);
    od_uart_send(buffer, to - from);
}
void dw_ll_write_registers_multi(uint8_t from, uint16_t len, ...){
    va_list ptr;
    va_start(ptr, len);
    dw_ll_reg_rw_setup(from, from + len, DW_MODE_REGS_WRITE);
    while(len--) od_uart_tx_byte((uint8_t) va_arg(ptr, int));
    va_end(ptr);
}

static void dw_ll_mem_rw_setup(uint16_t from, uint16_t len, uint8_t target){
    dw_ll_write_registers(30, 32, &from);
    len *= 2;
    if(target == DW_MODE_SRAM_WRITE)
        len += 1;
    dw_cmd_set_multi(DW_CMD_REG_PC, 0, len & 1);
    dw_cmd_set_multi(DW_CMD_REG_HWBP, len >> 8, len & 0xFF);
    dw_cmd_mode_set(target);
    dw_cmd_start_mem_cycle();
}
void dw_ll_read_sram(uint16_t from, uint16_t len, void * buffer){
    dw_ll_mem_rw_setup(from, len, DW_MODE_SRAM_READ);
    od_uart_recv(buffer, len);
}
void dw_ll_write_sram(uint16_t from, uint16_t len, void * buffer){
    dw_ll_mem_rw_setup(from, len, DW_MODE_SRAM_WRITE);
    od_uart_send(buffer, len);
}

void dw_ll_read_flash(uint16_t from, uint16_t len, void * buffer){
    dw_ll_mem_rw_setup(from, len, DW_MODE_FLASH_READ);
    if(buffer == NULL)
        od_uart_wait_until(len);
    else
        od_uart_recv(buffer, len);
}

static void dw_ll_flash_buffer_reset(void){
    uint16_t flash_end = debug_wire_g.device.flash_end - debug_wire_g.device.flash_page_end + 2;
    flash_end = BE(flash_end);

    dw_cmd_set(DW_CMD_REG_PC, &flash_end);
    dw_ll_exec(AVR_INSTR_LDI(r28, 0x11), 0);
    dw_ll_exec(AVR_INSTR_OUT(debug_wire_g.device.reg_spmcsr, r28), 0);
    dw_ll_exec(AVR_INSTR_SPM(), 1);
}

void dw_ll_clear_flash_page(uint16_t address){
    uint16_t flash_end = debug_wire_g.device.flash_end - debug_wire_g.device.flash_page_end + 2;
    flash_end = BE(flash_end);

    dw_ll_write_registers_multi(reg_X, 6, 0x03, 0x01, 0x05, 0x40, MULTI_LE(address));

    dw_cmd_set(DW_CMD_REG_PC, &flash_end); //set pc to boot section
    dw_set_context(DW_GO_CNTX_FLASH_WRT);
    dw_ll_exec(BE(AVR_INSTR_MOVW(r24, r30)), 0);
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_spmcsr, r26)), 0);
    dw_ll_exec(BE(AVR_INSTR_SPM()), 1);
}

uint8_t dw_ll_write_flash_page_begin(uint16_t address){

    dw_ll_clear_flash_page(address);
    dw_set_context(DW_GO_CNTX_FLASH_WRT & ~(1 << 5));
    _delay_ms(10);
    return debug_wire_g.device.flash_page_end;
}

uint8_t dw_ll_write_flash_populate_buffer(const uint16_t * buffer, uint16_t len, uint16_t remaining){
    uint16_t flash_end = debug_wire_g.device.flash_end - debug_wire_g.device.flash_page_end + 2;
    flash_end = BE(flash_end);

    len = len < remaining ? len : remaining;
    while(len--){
        dw_cmd_set(DW_CMD_REG_PC, &flash_end);
        dw_ll_exec(BE(AVR_INSTR_IN(r0, debug_wire_g.device.reg_dwdr)), 0); od_uart_tx_byte(*buffer & 0xff);
        dw_ll_exec(BE(AVR_INSTR_IN(r1, debug_wire_g.device.reg_dwdr)), 0); od_uart_tx_byte(*buffer >> 8);

        dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_spmcsr, r27)), 0);
        dw_ll_exec(BE(AVR_INSTR_SPM()), 0);
        dw_ll_exec(BE(AVR_INSTR_ADIW(adiw_reg_Z, 2)), 0);
        buffer += 1;
    }
    return len - remaining;
}

void dw_ll_write_flash_execute(void){
    uint16_t flash_end = debug_wire_g.device.flash_end - debug_wire_g.device.flash_page_end + 2;
    flash_end = BE(flash_end);

    dw_cmd_set(DW_CMD_REG_PC, &flash_end);
    dw_ll_exec(BE(AVR_INSTR_MOVW(r30, r24)), 0);
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_spmcsr, r28)), 0);
    dw_ll_exec(BE(AVR_INSTR_SPM()), 1);

    dw_ll_flash_buffer_reset();
}