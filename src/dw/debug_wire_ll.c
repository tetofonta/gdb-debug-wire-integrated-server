//
// Created by stefano on 31/07/22.
//http://www.ruemohr.org/docs/debugwire.html

#include <avr/io.h>
#include <util/delay.h>
#include <dw/debug_wire_ll.h>
#include <dw/debug_wire.h>
#include <dw/devices.h>
#include <panic.h>
#include <string.h>
#include "avr_isa.h"
#include "leds.h"
#include "usb/usb_cdc.h"
#include "gdb/gdb.h"
#include "gdb/utils.h"

debug_wire_t debug_wire_g;

static volatile uint8_t waiting_break, ignore_break, expect_break;
//=========================================================================================================base commands

/**
 * initializes the debug wire strucure, uart side
 * then if not already initialized, gets target signature and loads its descriptor.
 * @param target_freq target frequency
 * @return
 */
uint8_t dw_init(uint32_t target_freq) {
    cli();
    debug_wire_g.target_frequency = target_freq;
    od_uart_clear();
    od_uart_init(target_freq >> 7); //init uart as target freq/128 baud as from specifications
    sei();

    if(debug_wire_g.device.signature == 0x00){ //if not already initialized
        debug_wire_g.device.signature = 0x01; //no infinite recursion
        debug_wire_g.swbrkpt_n = 0;
        if(!dw_cmd_halt()) return 0;
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

void dw_ll_deinit(void){
    od_uart_clear();
    od_uart_deinit();
    debug_wire_g.device.signature = 0;
}

/**
 * send a break and waits for target response until timeout (100ms).
 * if not answered, fails
 * else checks for the correct answer and sets the baudrate.
 * it is not a real timeout, just waits before receiving.
 * @param timeout timeout in frames
 * @return
 */
uint8_t dw_cmd_halt(void){
    od_uart_clear(); //clear current uart rx
    dw_init(debug_wire_g.target_frequency);

    expect_break = 1;
    od_uart_break(); //send break as for protocol

    //vi sono due tipi di break:
    // nel caso di halt da stato runnning: il controllore risponde dopo la fine del break e ritorna 0x55
    // nel caso di halt inatteso (già halted) il controllore asserisce oltre la durata causando un interrupt startbit
    uint16_t timeout = 500;
    uint8_t ret = od_uart_recv_byte_timeout(&timeout);
    if(!timeout) return 0;
    if(ret != 0x55) ret = od_uart_recv_byte();
    if(ret != 0x55) return 0; //wrong answer

    debug_wire_g.halted = 1; //set state as halted
    MUST_SUCCEED(dw_cmd_set_speed(DW_DIVISOR_128), 1); //sets the speed
    debug_wire_g.program_counter = dw_cmd_get(DW_CMD_REG_PC); //save the program counter
    DW_LED_ON();
    return 1;
}

/**
 * sends a command and gets the 16bit answer
 * @param cmd
 * @return
 */
uint16_t dw_cmd_get_16(uint8_t cmd){
    uint16_t ret;
    dw_cmd(cmd);
    od_uart_recv(&ret, 2);
    return ret;
}

/**
 * sends a command and gets the 8bit answer
 * @param cmd
 * @return
 */
uint8_t dw_cmd_get_8(uint8_t cmd){
    od_uart_clear();
    dw_cmd(cmd);
    return od_uart_recv_byte();
}

/**
 * sets the speed as the specified divisor
 * @param divisor DW_DIVISOR_*
 * @return
 */
uint8_t dw_cmd_set_speed(uint8_t divisor){
    debug_wire_g.cur_divisor = divisor;
    return dw_cmd_get_8(divisor) == 0x55;
}

/**
 * sends multiple data stored in a buffer of len len
 * @param command
 * @param data
 * @param len
 */
void dw_cmd_send_multiple(uint8_t command, void * data, uint8_t len){
    dw_cmd(command);
    od_uart_send(data, len);
}

/**
 * sends multiple data after a command by specifying varargs consts
 * @param command
 * @param n
 * @param ...
 */
void dw_cmd_send_multiple_consts(uint8_t command, uint16_t n, ...){
    va_list ptr;
    va_start(ptr, n);
    dw_cmd(command);
    while(n--)od_uart_tx_byte((uint8_t) va_arg(ptr, int));
    va_end(ptr);
}

/**
 * resets the target and waits for target initiated break.
 */
void dw_cmd_reset(void){
    od_uart_clear();
    waiting_break = 1;
    expect_break = 1;
    dw_cmd(DW_CMD_RESET);
    while(waiting_break);
}

/**
 * makes the target continue execution form flash or from loaded ir if is a swbrkpt
 * @param is_sw_brkpt
 */
void dw_cmd_go(uint8_t is_sw_brkpt){
    debug_wire_g.halted = 0;
    DW_LED_OFF();
    dw_cmd(is_sw_brkpt ? DW_CMD_GO_FROM_IR : DW_CMD_GO);
}

//==================================================================================================================irqs
inline void od_uart_irq_break(void){
    od_uart_blank(1); //blank for one frame
    od_uart_clear(); //clear uart
    dw_init(debug_wire_g.target_frequency); //reset baudrate
    od_uart_blank(8); //wait for things to settle down
    od_uart_clear();
    dw_cmd_set_speed(debug_wire_g.cur_divisor); //set speed
    if(ignore_break){ //if break is expected as part of an operation, clear and return
        ignore_break = 0;
        waiting_break = 0;
        return;
    }
    uint16_t pc = dw_cmd_get(DW_CMD_REG_PC); //F0 hh ll -- get the PC. TAKE NOTE after a break this will return PC+1.
    pc = BE(pc) - 1;
    debug_wire_g.program_counter = BE(pc); //save the program counter
    debug_wire_g.halted = 1; //set status as halted
    DW_LED_ON();
    if(!expect_break) on_dw_mcu_halt();
    expect_break = 0;
    waiting_break = 0; //clear and return
}

//===================================================================================================low level functions

/**
 * loadas an instruction and executes it.
 * USES:
 *  - IR
 * @param instruction instruction opcode
 * @param long_instruction if true will wait for a break after execution. cpu will halt until executed. (spm ecc)
 */
void dw_ll_exec(uint16_t instruction, uint8_t long_instruction){
    dw_cmd_set(DW_CMD_REG_IR, &instruction);                            //; SET IR = opcode
    if(long_instruction){
        waiting_break = 1;
        ignore_break = 1;
        dw_cmd(DW_CMD_EXECUTE_LOADED_INSTR_LNG);                        //; execute long
        while(waiting_break);
    } else {                                                            //or
        dw_cmd(DW_CMD_EXECUTE_LOADED_INSTR);                            //; execute
    }
}

/**
 * setups a register file write
 * USES:
 *  - PC
 *  - HWBP
 *
 * @param from register where to start
 * @param to register where to end (included)
 * @param target dw mode DW_MODE_*
 */
static void dw_ll_reg_rw_setup(uint8_t from, uint8_t to, uint8_t target){
    uint16_t from_be = from << 8 | from >> 8;
    uint16_t to_be = (to + 1) << 8 | (to + 1) >> 8;

    dw_set_context(DW_GO_CNTX_MEM);             //set context to memory operation
    dw_cmd_set(DW_CMD_REG_PC, &from_be);        //set starting register address
    dw_cmd_set(DW_CMD_REG_HWBP, &to_be);        //set finish register address (to + 1)
    dw_cmd_mode_set(target);                    //set mode
    if(target == DW_MODE_REGS_WRITE && (to - from + 1) == 1)    //if writing only one register use ss
        dw_cmd_start_mem_cycle_ss();
    else
        dw_cmd_start_mem_cycle();
}
/**
 * reads one register
 * USES:
 *  - PC   | from dw_ll_reg_rw_setup
 *  - HWBP |
 * @param reg register
 * @return
 */
uint8_t dw_ll_register_read(uint8_t reg){
    od_uart_clear();
    dw_ll_reg_rw_setup(reg, reg, DW_MODE_REGS_READ);
    return od_uart_recv_byte();
}
/**
 * writes one register
 * USES:
 *  - PC   | from dw_ll_reg_rw_setup
 *  - HWBP |
 * @param reg register
 * @param data byute to write
 * @return
 */
void dw_ll_register_write(uint8_t reg, uint8_t data){
    dw_ll_reg_rw_setup(reg, reg, DW_MODE_REGS_WRITE);
    od_uart_tx_byte(data);
}
/**
 * reads the register file register
 * USES:
 *  - PC   | from dw_ll_reg_rw_setup
 *  - HWBP |
 * @param from register where to start
 * @param to register where to end (included)
 * @param buffer: where to store the data read
 * @return
 */
void dw_ll_registers_read(uint8_t from, uint8_t to, void * buffer){
    dw_ll_reg_rw_setup(from, to, DW_MODE_REGS_READ);
    od_uart_recv(buffer, to - from + 1);
}
/**
 * writes the register file register
 * USES:
 *  - PC   | from dw_ll_reg_rw_setup
 *  - HWBP |
 * @param from register where to start
 * @param to register where to end (included)
 * @param buffer: data to be written
 * @return
 */
void dw_ll_registers_write(uint8_t from, uint8_t to, const void * buffer){
    dw_ll_reg_rw_setup(from, to, DW_MODE_REGS_WRITE);
    od_uart_send(buffer, to - from + 1);
}
/**
 * write the register file register from a const varag list
 * USES:
 *  - PC   | from dw_ll_reg_rw_setup
 *  - HWBP |
 * @param from register where to start
 * @param len amount of registers to be written
 * @param ... consts
 * @return
 */
void dw_ll_registers_write_multi(uint8_t from, uint16_t len, ...){
    va_list ptr;
    va_start(ptr, len);
    dw_ll_reg_rw_setup(from, from + len - 1, DW_MODE_REGS_WRITE);
    while(len--) od_uart_tx_byte((uint8_t) va_arg(ptr, int));
    va_end(ptr);
}

/**
 * setups a memory rw operation, operates on all the memory (register file in address space),
 * so i/o starts at 0x20 and real sram base is specified in device descriptor
 * USES:
 *  - PC
 *  - HWBP
 *  - Z (r30 and r31)
 *
 * @param from address where to start
 * @param len
 * @param target
 */
static void dw_ll_mem_rw_setup(uint16_t from, uint16_t len, uint8_t target){
    dw_ll_registers_write(r30, r31, &from);         //set address to Z
    len *= 2;
    if(target == DW_MODE_SRAM_WRITE)                                //if sram write increment start and end by one
        len += 1;
    dw_cmd_set_multi(DW_CMD_REG_PC, 0, len & 1);                    //set pc = 0 or 1
    dw_cmd_set_multi(DW_CMD_REG_HWBP, len >> 8, len & 0xFF);        //set operation len (2*)
    dw_cmd_mode_set(target);                                        //set len
    dw_cmd_start_mem_cycle();                                       //start
}
/**
 * reads sram and stores i  buffer
 * operates on all the memory (register file in address space),
 * so i/o starts at 0x20 and real sram base is specified in device descriptor
 * USES:
 *  - PC
 *  - HWBP
 *  - Z (r30 and r31)
 *
 * @param from
 * @param len
 * @param buffer
 */
void dw_ll_sram_read(uint16_t from, uint16_t len, void * buffer){
    dw_ll_mem_rw_setup(from, len, DW_MODE_SRAM_READ);
    od_uart_recv(buffer, len);
}
/**
 * write sram from buffer at address
 * operates on all the memory (register file in address space),
 * so i/o starts at 0x20 and real sram base is specified in device descriptor
 * USES:
 *  - PC
 *  - HWBP
 *  - Z (r30 and r31)
 *
 * @param from
 * @param len
 * @param buffer
 */
void dw_ll_sram_write(uint16_t from, uint16_t len, void * buffer){
    dw_ll_mem_rw_setup(from, len, DW_MODE_SRAM_WRITE);
    od_uart_send(buffer, len);
}

/**
 * performs a page reset or RWWSRE
 * if the target does not support rww the spmcsr bit will be a buffer clear.
 *
 * USES:
 *  - PC
 *  - IR
 *  - r28
 */
void dw_ll_enable_rww(void){
    uint16_t flash_end = debug_wire_g.device.flash_end - debug_wire_g.device.flash_page_end + 2;
    flash_end = BE(flash_end);

    dw_cmd_set(DW_CMD_REG_PC, &flash_end);
    dw_ll_exec(BE(AVR_INSTR_LDI(r28, 0x11)), 0);
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_spmcsr, r28)), 0);
    dw_ll_exec(BE(AVR_INSTR_SPM()), 1);
}
/**
 * read flash memory from address (from) and for given len, stores in the buffer.
 * if buffer is NULL, data won't be cleared from uart recv.
 * USES:
 *  - PC
 *  - HWBP
 *  - Z (r30, r31)
 * @param from
 * @param len
 * @param buffer
 */
void dw_ll_flash_read(uint16_t from, uint16_t len, void * buffer){
    dw_ll_mem_rw_setup(from, len, DW_MODE_FLASH_READ);
    if(buffer == NULL)
        od_uart_wait_until(len);
    else
        od_uart_recv(buffer, len);
}
/**
 * clear flash page at address (real byte address, it will be (device.flash_page_end * 2 * page_no)).
 * USES:
 *  - PC
 *  - HWBP
 *  - Z (r30, r31)
 *  - X (r26, r27)
 *  - Y (r28, r29)
 *  - IR
 * @param address
 */
void dw_ll_flash_clear_page(uint16_t address){
    uint16_t flash_end = debug_wire_g.device.flash_end - debug_wire_g.device.flash_page_end + 2;
    flash_end = BE(flash_end);

    dw_ll_registers_write_multi(reg_X, 6, 0x03, 0x01, 0x05, 0x40, MULTI_LE(address));

    dw_cmd_set(DW_CMD_REG_PC, &flash_end); //set pc to boot section
    dw_set_context(DW_GO_CNTX_FLASH_WRT);
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_spmcsr, r26)), 0);
    dw_ll_exec(BE(AVR_INSTR_SPM()), 1);
}
/**
 * starts a page write at address (device.flash_page_end * 2 * page_no):
 * performs a page erase and prepares for buffer fill
 * USES:
 *  - PC
 *  - HWBP
 *  - X (r26, r27)
 *  - Y (r28, r29)
 *  - Z (r30, r31)
 *  - IR
 *  - r28
 *
 * Example usage:
 * uint16_t rem = dw_ll_flash_write_page_begin(address);
 * while((rem = dw_ll_flash_write_populate_buffer(get_data(), data_len, rem)));
 * dw_ll_flash_clear_page();
 * dw_ll_flash_write_execute();
 *
 * @param address
 * @return the amount of words the buffer will contains.
 */
uint8_t dw_ll_flash_write_page_begin(uint16_t address){
    dw_set_context(DW_GO_CNTX_FLASH_WRT & ~(1 << 5));
    dw_ll_enable_rww();
    dw_ll_registers_write_multi(reg_X, 6, 0x03, 0x01, 0x05, 0x40, MULTI_LE(address));
    return debug_wire_g.device.flash_page_end;
}
/**
 * see dw_ll_flash_write_page_begin
 * populates target page buffer from given buffer with min(len, remaining) bytes;
 * USES:
 *  - PC
 *  - HWBP
 *  - X (r26, r27)
 *  - Y (r28, r29)
 *  - Z (r30, r31)
 *  - IR
 *  - r0
 *  - r1
 * @param buffer
 * @param len
 * @param remaining
 * @return
 */
uint8_t dw_ll_flash_write_populate_buffer(const uint16_t * buffer, uint16_t len, uint16_t remaining){
    uint16_t flash_end = debug_wire_g.device.flash_end - debug_wire_g.device.flash_page_end + 2;
    flash_end = BE(flash_end);

    uint16_t tmp = len < remaining ? len : remaining;
    len = tmp;
    while(len--){
        dw_cmd_set(DW_CMD_REG_PC, &flash_end);
        dw_ll_exec(BE(AVR_INSTR_IN(r0, debug_wire_g.device.reg_dwdr)), 0); od_uart_tx_byte(*buffer & 0xff);
        dw_ll_exec(BE(AVR_INSTR_IN(r1, debug_wire_g.device.reg_dwdr)), 0); od_uart_tx_byte(*buffer >> 8);
        dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_spmcsr, r27)), 0);
        dw_ll_exec(BE(AVR_INSTR_SPM()), 0);
        dw_ll_exec(BE(AVR_INSTR_ADIW(adiw_reg_Z, 2)), 0);
        buffer += 1;
    }
    return remaining - tmp;
}
/**
 * see dw_ll_flash_write_page_begin
 * executes page write
 * USES:
 *  - PC
 *  - HWBP
 *  - X (r26, r27)
 *  - Y (r28, r29)
 *  - Z (r30, r31)
 *  - IR
 *  - r28
 * @param buffer
 * @param len
 * @param remaining
 * @return
 */
void dw_ll_flash_write_execute(void){
    uint16_t flash_end = debug_wire_g.device.flash_end - debug_wire_g.device.flash_page_end + 2;
    flash_end = BE(flash_end);

    dw_cmd_set(DW_CMD_REG_PC, &flash_end);

    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_spmcsr, r28)), 0);
    dw_ll_exec(BE(AVR_INSTR_SPM()), 1);
    _delay_ms(50);
    dw_ll_enable_rww();
}

/**
 * reads a byte from eeprom at address
 * USES:
 *  - PC
 *  - HWBP
 *  - Y (r28, r29)
 *  - Z (r30, r31)
 *  - IR
 *  - r0
 * @param address
 * @return
 */
uint8_t dw_ll_eeprom_read_byte(uint16_t address){
    dw_ll_registers_write_multi(reg_Y, 4, 0x01, 0x01, MULTI_LE(address));
    dw_set_context(DW_GO_CNTX_FLASH_WRT & ~(1 << 5));
    dw_ll_exec(BE(AVR_INSTR_OUT((debug_wire_g.device.reg_eearl + 1), r31)), 0);
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_eearl, r30)), 0);
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_eecr, r28)), 0);
    dw_ll_exec(BE(AVR_INSTR_IN(r0, debug_wire_g.device.reg_eedr)), 0);

    od_uart_clear();
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_dwdr, r0)), 0);
    return od_uart_recv_byte();
}
/**
 * reads data from eeprom at address for len and stores it in buffer
 * USES:
 *  - PC
 *  - HWBP
 *  - Y (r28, r29)
 *  - Z (r30, r31)
 *  - IR
 *  - r0
 * @param buffer
 * @param address
 * @param len
 */
void dw_ll_eeprom_read(void * buffer, uint16_t address, uint16_t len){
    while(len--) *((uint8_t *) buffer++) = dw_ll_eeprom_read_byte(address++);
}
/**
 * writes data to eeprom at address
 * USES:
 *  - PC
 *  - HWBP
 *  - X (r26, r27)
 *  - Y (r28, r29)
 *  - Z (r30, r31)
 *  - IR
 *  - r0
 * @param address
 * @param data
 */
void dw_ll_eeprom_write_byte(uint16_t address, uint8_t data){
    dw_ll_registers_write_multi(reg_X, 6, 0x04, 0x02, 0x01, 0x01, MULTI_LE(address));
    dw_set_context(DW_GO_CNTX_FLASH_WRT & ~(1 << 5));
    dw_ll_exec(BE(AVR_INSTR_OUT((debug_wire_g.device.reg_eearl + 1), r31)), 0);
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_eearl, r30)), 0);
    dw_ll_exec(BE(AVR_INSTR_IN(r0, debug_wire_g.device.reg_dwdr)), 0);
    od_uart_tx_byte(data);
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_eedr, r0)), 0);
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_eecr, r26)), 0);
    dw_ll_exec(BE(AVR_INSTR_OUT(debug_wire_g.device.reg_eecr, r27)), 0);
}
/**
 * writes data to eeprom at address and for len from buffer
 * USES:
 *  - PC
 *  - HWBP
 *  - X (r26, r27)
 *  - Y (r28, r29)
 *  - Z (r30, r31)
 *  - IR
 *  - r0
 * @param buffer
 * @param address
 * @param len
 */
void dw_ll_eeprom_write(const void * buffer, uint16_t address, uint16_t len){
    while(len--) dw_ll_eeprom_write_byte(address++, *((uint8_t *) buffer++));
}

//===========================================================================================================breakpoints
uint8_t dw_ll_add_breakpoint(uint16_t word_address){
    for (uint8_t i = 0; i < debug_wire_g.swbrkpt_n; ++i)
        if (debug_wire_g.swbrkpt[i].address == word_address){
            debug_wire_g.swbrkpt[i].active = 1;
            return 1;
        }

    if(debug_wire_g.swbrkpt_n == DW_SW_BRKPT_SIZE) return 0;

    dw_sw_brkpt_t  * bp = &debug_wire_g.swbrkpt[debug_wire_g.swbrkpt_n++];
    bp->address = word_address;
    bp->active = 1;
    bp->stored = 0;
    return 1;
}

uint8_t dw_ll_remove_breakpoint(uint16_t word_address){
    for (uint8_t i = 0; i < debug_wire_g.swbrkpt_n; ++i)
        if (debug_wire_g.swbrkpt[i].address == word_address){
            debug_wire_g.swbrkpt[i].active = 0;
            return 1;
        }
    return 0;
}

void dw_ll_clear_breakpoints(void){
    for (uint8_t i = 0; i < debug_wire_g.swbrkpt_n; ++i)
        debug_wire_g.swbrkpt[i].active = 0;
}

static inline uint8_t is_bp_addr_gt(dw_sw_brkpt_t * a, dw_sw_brkpt_t * b){
    //da attivare active && !stored
    //da rimuovere !active && stored
    //attivo active && stored
    //rimosso !active && !stored
    if(!a->active && !a->stored && !b->active && !b->stored) return 0; //se sono entrambi rimossi va bene così
    if(!a->active && !a->stored && (b->active || b->stored)) return 1; //se a è rimosso e b esiste -> a è sempre maggiore
    if((a->active || a->stored) && !b->active && !b->stored) return 0; //se a esiste e b è rimosso -> a è sempre minore
    return a->address > b->address;
}

static void dw_ll_internal_update_bp_references(void){
    if(debug_wire_g.swbrkpt_n < 2) {
        if(!debug_wire_g.swbrkpt[0].active) debug_wire_g.swbrkpt_n = 0;
        return;
    }
    //performs an insertion sort
    dw_sw_brkpt_t tmp;
    for (int16_t i = 1; i < debug_wire_g.swbrkpt_n; ++i) {
        memcpy(&tmp, debug_wire_g.swbrkpt + i, sizeof(dw_sw_brkpt_t));
        int16_t j = i - 1;
        while(j >= 0 && is_bp_addr_gt(&debug_wire_g.swbrkpt[j], &tmp)){
            memcpy(debug_wire_g.swbrkpt + j + 1, debug_wire_g.swbrkpt + j, sizeof(dw_sw_brkpt_t));
            j--;
        }
        memcpy(debug_wire_g.swbrkpt + j + 1, &tmp, sizeof(dw_sw_brkpt_t));
    }

    while(!debug_wire_g.swbrkpt[debug_wire_g.swbrkpt_n - 1].active && !debug_wire_g.swbrkpt[debug_wire_g.swbrkpt_n - 1].stored && debug_wire_g.swbrkpt_n > 0) debug_wire_g.swbrkpt_n--;
}

uint16_t break_opcode = AVR_INSTR_BREAK(); //little-endian in flash
uint16_t byte_address;
uint16_t read;
uint16_t z;
uint16_t cur_bp_offset;
uint8_t executed;
uint8_t written;

static uint8_t dw_ll_internal_write_breakpoints(uint16_t page_address, dw_sw_brkpt_t * bps, uint16_t bps_size, uint16_t * buffer, uint16_t buf_size){

    byte_address = page_address * 2;
    read = 0;
    uint16_t remaining_words = dw_ll_flash_write_page_begin(byte_address);
    cur_bp_offset = bps->address - page_address;
    executed = 0;
    written = 0;


    while(remaining_words){
        uint16_t words_to_read = (buf_size < remaining_words ? buf_size : remaining_words);

        dw_ll_registers_read(r30, r31, &z);
        dw_ll_flash_read(byte_address + read, words_to_read * 2, (uint8_t *) buffer);//128-64 //read this part of the word
        dw_ll_registers_write(r30, r31, &z);

        uint16_t cur_buffer_wrt = 0;
        while (cur_bp_offset < words_to_read){
            executed++;
            remaining_words = dw_ll_flash_write_populate_buffer(buffer + cur_buffer_wrt, cur_bp_offset - cur_buffer_wrt, remaining_words);

            if(bps->active && !bps->stored){
                uint32_t ad = byte2hex(bps->address & 0xFF) |  (uint32_t) byte2hex(bps->address >> 8) << 16;
                gdb_message((const char *) &ad, PSTR("O57524f5445"), 4, 11);
                bps->stored = 1;
                (bps++)->opcode  = *(buffer + cur_bp_offset);
                remaining_words = dw_ll_flash_write_populate_buffer(&break_opcode, 1, remaining_words);
                written++;
            } else if(!bps->active && bps->stored) {
                uint32_t ad = byte2hex(bps->address & 0xFF) |  (uint32_t) byte2hex(bps->address >> 8) << 16;
                gdb_message((const char *) &ad, PSTR("O524f4d4f56"), 4, 11);
                bps->stored = 0;
                remaining_words = dw_ll_flash_write_populate_buffer(&(bps++)->opcode, 1, remaining_words);
                written++;
            } else {
                remaining_words = dw_ll_flash_write_populate_buffer(buffer + cur_bp_offset, 1, remaining_words);
            }

            cur_buffer_wrt = cur_bp_offset + 1;
            if(!--bps_size) break;
            cur_bp_offset = bps->address - page_address - read/2;
        }
        remaining_words = dw_ll_flash_write_populate_buffer(buffer + cur_buffer_wrt, words_to_read - cur_buffer_wrt, remaining_words);

        read += words_to_read * 2;
        cur_bp_offset -= words_to_read;
    }

    if(written){
        _delay_ms(50);
        dw_ll_flash_clear_page(byte_address);
        _delay_ms(50);
        dw_ll_flash_write_execute();
    }

    return executed;
}

void dw_ll_flush_breakpoints(uint16_t * buffer, uint16_t len){
    dw_ll_internal_update_bp_references();

    uint8_t bp_len = 0;
    while(bp_len < debug_wire_g.swbrkpt_n){
        uint16_t page_address = ((((debug_wire_g.swbrkpt + bp_len)->address) / debug_wire_g.device.flash_page_end) * debug_wire_g.device.flash_page_end);
        bp_len += dw_ll_internal_write_breakpoints(page_address, debug_wire_g.swbrkpt + bp_len, debug_wire_g.swbrkpt_n - bp_len, buffer, len);
    }
}