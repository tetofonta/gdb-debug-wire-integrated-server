//
// Created by stefano on 15/08/22.
//
#include <avr_isa.h>

bool illegal_opcode(uint16_t opcode){
    uint8_t family = (opcode & 0xFF00) >> 8;
    if(family == 0x00) return opcode != 0; //nop
    if((family & 0xFE) == 0x90) return (opcode & 0x0F) == 3 || (opcode & 0x0F) == 8 || (opcode & 0x0F) == 0x0B; //0x90 or 0x91
    if((family & 0xFC) == 0x90) return ((opcode & 0x0F) >= 3 && (opcode & 0x0F) <=8) || (opcode & 0x0F) == 0x0B; //0x92 or 0x93
    if((family & 0xFE) == 0x94){ //94 95
        if((opcode & 0x0F) == 4) return true;
        if((opcode & 0x0F) == 9 && (opcode & 0xE0) != 0) return true;
        if(family == 0x94 && (opcode & 0x0F) == 0x0B) return true;
        if(family == 0x95)
            return (opcode & 0xF0) == 0 || (opcode & 0xF0) == 0x10 || (opcode & 0xF0) == 0x80 || (opcode & 0xF0) == 0xA0 || ((opcode & 0xF0) >= 0xC0 && (opcode & 0xF0) <= 0xE0);
    }
    if(family >= 0xF8) return opcode & 0x07;
    return false;
}


//00xx -> only 0000 is valid
//90xx -> 90x3 90x8 90xB illegal
//91xx -> 91x3 91x8 91xB illegal
//          91E1 91e2 91e5 91e7 91f1 91f2 91f5 91f7 91c9 91ca 91d9 91da 91ad 91ae 91bd 91be undefined -- not handled =(
//92xx -> 92x3 92x4 91x5 92x6 92x7 92x8 92xb illegal
//93xx -> 93x3 93x4 93x5 93x6 93x7 93x8 93xb illegal
//94xx -> 94x4 94x9\{9409 9419} 94xB
//95xx -> 95x4 95x9\{9509 9519} 95x8\{9508 9518 9588 9598 95a8 95c8 95d8 95e8}
//f8xx -> f8x{b11111xxx}
//same until ffxx