//
// Created by stefano on 06/08/22.
//

#ifndef ARDWINO_AVR_ISA_H
#define ARDWINO_AVR_ISA_H

#define BE(x)                               (x << 8 | x >> 8)
#define MULTI_BE(x)                         (x >> 8), (x & 0xFF)
#define MULTI_LE(x)                         (x & 0xFF), (x >> 8)

#define AVR_INSTR_OUT(A, r)                 ((uint16_t) (0b1011100000000000 | ((r & 0x1F) << 4) | (A & 0x0F) | ((A & 0x30) << 5)))
#define AVR_INSTR_IN(r, A)                  ((uint16_t) (0b1011000000000000 | ((r & 0x1F) << 4) | (A & 0x0F) | ((A & 0x30) << 5)))
#define AVR_INSTR_MOVW(wd, wr)              ((uint16_t) (0b0000000100000000 | (((wd >> 1) & 0x0F) << 4) | ((wr >> 1) & 0x0F)))
#define AVR_INSTR_SPM()                     ((uint16_t) (0b1001010111101000))
#define AVR_INSTR_ADIW(w, K)                ((uint16_t) (0b1001011000000000 | ((K & 0x30) << 2) | (K & 0x0F) | ((w & 0x03) << 4)))
#define AVR_INSTR_LDI(rd, K)                ((uint16_t) (0b1110000000000000 | ((K & 0xF0) << 8) | (K & 0x0F) | ((rd & 0x0F) << 4)))
#define AVR_INSTR_BREAK()                   ((uint16_t) (0b1001010110011000))

#define r0                                  0
#define r1                                  1
#define r2                                  2
#define r3                                  3
#define r4                                  4
#define r5                                  5
#define r6                                  6
#define r7                                  7
#define r8                                  8
#define r9                                  9
#define r10                                 10
#define r11                                 11
#define r12                                 12
#define r13                                 13
#define r14                                 14
#define r15                                 15
#define r16                                 16
#define r17                                 17
#define r18                                 18
#define r19                                 19
#define r20                                 20
#define r21                                 21
#define r22                                 22
#define r23                                 23
#define r24                                 24
#define r25                                 25
#define r26                                 26
#define r27                                 27
#define r28                                 28
#define r29                                 29
#define r30                                 30
#define r31                                 31

#define reg_X                               r26
#define reg_Y                               r28
#define reg_Z                               r30

#define adiw_reg_X                               1
#define adiw_reg_Y                               2
#define adiw_reg_Z                               3


#endif //ARDWINO_AVR_ISA_H
