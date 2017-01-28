//
//  DD6502.c
//  DDNES
//
//  Created by David Kopec on 1/26/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include "DD6502.h"

struct {
    byte a; // accumulator
    byte x;
    byte y;
    byte sp; // stack pointer
    word pc; // program counter
} registers;

struct {
    bool c; // carry
    bool z; // zero
    bool i; // interrupt disable
    bool d; // decimal mode
    bool b; // break command
    bool o; // overflow
    bool n; // negative
} flags;

const int MEM_SIZE = 65535;
byte ram[MEM_SIZE]; // memory - 64k

void cpu_reset() {
    // reset registers
    registers.a = 0;
    registers.x = 0;
    registers.y = 0;
    registers.sp = 0;
    registers.pc = 0;
    // reset flags
    flags.c = false;
    flags.z = false;
    flags.i = false;
    flags.d = false;
    flags.b = false;
    flags.o = false;
    flags.n = false;
}
