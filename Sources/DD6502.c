//
//  DD6502.c
//  DDNES
//
//  Created by David Kopec on 1/26/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include "DD6502.h"

// Based on data about opcodes from:
// http://www.atariarchives.org/mlb/appendix_a.php
// http://www.thealmightyguru.com/Games/Hacking/Wiki/index.php/6502_Opcodes
// http://nesdev.com/6502.txt
// http://www.6502.org/tutorials/6502opcodes.html#STACK
// https://github.com/fogleman/nes/blob/master/nes/cpu.go

struct {
    byte a; // accumulator
    byte x;
    byte y;
    byte sp; // stack pointer
    word pc; // program counter
} registers;

#define A registers.a
#define X registers.x
#define Y registers.y
#define SP registers.sp
#define PC registers.pc


struct {
    bool c; // carry
    bool z; // zero
    bool i; // interrupt disable
    bool d; // decimal mode
    bool b; // break command
    bool v; // oVerflow http://www.6502.org/tutorials/vflag.html
    bool n; // negative
} flags;

#define C flags.c
#define Z flags.z
#define I flags.i
#define D flags.d
#define B flags.b
#define V flags.v
#define N flags.n

// status regiser is a combo of flags
// the 5th one is a dummy 1
#define S (C | Z << 1 | I << 2 | D << 3 | B << 4 | 1 << 5 | V << 6 | N << 7)

#define DIFFERENT_PAGES(a1, a2) ((a1 & 0xFF00) != (a2 & 0xFF00))

#define STACK_START 0xFD
#define RESET_VECTOR 0xFFFC
#define NMI_VECTOR 0xFFFA
#define IRQ_BRK_VECTOR 0xFFFE

typedef enum {
    ADC,
    AHX,
    ALR,
    ANC,
    AND,
    ARR,
    ASL,
    AXS,
    BCC,
    BCS,
    BEQ,
    BIT,
    BMI,
    BNE,
    BPL,
    BRK,
    BVC,
    BVS,
    CLC,
    CLD,
    CLI,
    CLV,
    CMP,
    CPX,
    CPY,
    DCP,
    DEC,
    DEX,
    DEY,
    EOR,
    INC,
    INX,
    INY,
    ISC,
    JMP,
    JSR,
    KIL,
    LAS,
    LAX,
    LDA,
    LDX,
    LDY,
    LSR,
    NOP,
    ORA,
    PHA,
    PHP,
    PLA,
    PLP,
    RLA,
    ROL,
    ROR,
    RRA,
    RTI,
    RTS,
    SAX,
    SBC,
    SEC,
    SED,
    SEI,
    SHX,
    SHY,
    SLO,
    SRE,
    STA,
    STX,
    STY,
    TAS,
    TAX,
    TAY,
    TSX,
    TXA,
    TXS,
    TYA,
    XAA
} instruction_type;

typedef struct {
    instruction_type instruction; // the actual instruction (LDA)
    char name[4]; // string descriptor like "LDA"
    mem_mode mode; // memory access mode
    byte length; // how many bytes long is the instruction with its data
    byte ticks; // how many CPU cycles does the instruction need
    byte pageTicks; // how many CPU cycles does it take when a page is crossed
} instruction_info;

// the following giant table has been machine generated (see python script) from tables
// included in the NES emulator "nes" by Michael Fogleman (https://github.com/fogleman/nes)
// the instructions table is indexed by the instruction itself
instruction_info instructions[256] = {
    {BRK, "BRK", IMPLIED, 1, 7, 0}, 			// 00
    {ORA, "ORA", INDEXED_INDIRECT, 2, 6, 0}, 	// 01
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// 02
    {SLO, "SLO", INDEXED_INDIRECT, 0, 8, 0}, 	// 03
    {NOP, "NOP", ZEROPAGE, 2, 3, 0}, 			// 04
    {ORA, "ORA", ZEROPAGE, 2, 3, 0}, 			// 05
    {ASL, "ASL", ZEROPAGE, 2, 5, 0}, 			// 06
    {SLO, "SLO", ZEROPAGE, 0, 5, 0}, 			// 07
    {PHP, "PHP", IMPLIED, 1, 3, 0}, 			// 08
    {ORA, "ORA", IMMEDIATE, 2, 2, 0}, 			// 09
    {ASL, "ASL", ACCUMULATOR, 1, 2, 0}, 		// 0a
    {ANC, "ANC", IMMEDIATE, 0, 2, 0}, 			// 0b
    {NOP, "NOP", ABSOLUTE, 3, 4, 0}, 			// 0c
    {ORA, "ORA", ABSOLUTE, 3, 4, 0}, 			// 0d
    {ASL, "ASL", ABSOLUTE, 3, 6, 0}, 			// 0e
    {SLO, "SLO", ABSOLUTE, 0, 6, 0}, 			// 0f
    {BPL, "BPL", RELATIVE, 2, 2, 1}, 			// 10
    {ORA, "ORA", INDIRECT_INDEXED, 2, 5, 1}, 	// 11
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// 12
    {SLO, "SLO", INDIRECT_INDEXED, 0, 8, 0}, 	// 13
    {NOP, "NOP", ZEROPAGE_X, 2, 4, 0}, 			// 14
    {ORA, "ORA", ZEROPAGE_X, 2, 4, 0}, 			// 15
    {ASL, "ASL", ZEROPAGE_X, 2, 6, 0}, 			// 16
    {SLO, "SLO", ZEROPAGE_X, 0, 6, 0}, 			// 17
    {CLC, "CLC", IMPLIED, 1, 2, 0}, 			// 18
    {ORA, "ORA", ABSOLUTE_Y, 3, 4, 1}, 			// 19
    {NOP, "NOP", IMPLIED, 1, 2, 0}, 			// 1a
    {SLO, "SLO", ABSOLUTE_Y, 0, 7, 0}, 			// 1b
    {NOP, "NOP", ABSOLUTE_X, 3, 4, 1}, 			// 1c
    {ORA, "ORA", ABSOLUTE_X, 3, 4, 1}, 			// 1d
    {ASL, "ASL", ABSOLUTE_X, 3, 7, 0}, 			// 1e
    {SLO, "SLO", ABSOLUTE_X, 0, 7, 0}, 			// 1f
    {JSR, "JSR", ABSOLUTE, 3, 6, 0}, 			// 20
    {AND, "AND", INDEXED_INDIRECT, 2, 6, 0}, 	// 21
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// 22
    {RLA, "RLA", INDEXED_INDIRECT, 0, 8, 0}, 	// 23
    {BIT, "BIT", ZEROPAGE, 2, 3, 0}, 			// 24
    {AND, "AND", ZEROPAGE, 2, 3, 0}, 			// 25
    {ROL, "ROL", ZEROPAGE, 2, 5, 0}, 			// 26
    {RLA, "RLA", ZEROPAGE, 0, 5, 0}, 			// 27
    {PLP, "PLP", IMPLIED, 1, 4, 0}, 			// 28
    {AND, "AND", IMMEDIATE, 2, 2, 0}, 			// 29
    {ROL, "ROL", ACCUMULATOR, 1, 2, 0},         // 2a
    {ANC, "ANC", IMMEDIATE, 0, 2, 0}, 			// 2b
    {BIT, "BIT", ABSOLUTE, 3, 4, 0}, 			// 2c
    {AND, "AND", ABSOLUTE, 3, 4, 0}, 			// 2d
    {ROL, "ROL", ABSOLUTE, 3, 6, 0}, 			// 2e
    {RLA, "RLA", ABSOLUTE, 0, 6, 0}, 			// 2f
    {BMI, "BMI", RELATIVE, 2, 2, 1}, 			// 30
    {AND, "AND", INDIRECT_INDEXED, 2, 5, 1}, 	// 31
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// 32
    {RLA, "RLA", INDIRECT_INDEXED, 0, 8, 0}, 	// 33
    {NOP, "NOP", ZEROPAGE_X, 2, 4, 0}, 			// 34
    {AND, "AND", ZEROPAGE_X, 2, 4, 0}, 			// 35
    {ROL, "ROL", ZEROPAGE_X, 2, 6, 0}, 			// 36
    {RLA, "RLA", ZEROPAGE_X, 0, 6, 0}, 			// 37
    {SEC, "SEC", IMPLIED, 1, 2, 0}, 			// 38
    {AND, "AND", ABSOLUTE_Y, 3, 4, 1}, 			// 39
    {NOP, "NOP", IMPLIED, 1, 2, 0}, 			// 3a
    {RLA, "RLA", ABSOLUTE_Y, 0, 7, 0}, 			// 3b
    {NOP, "NOP", ABSOLUTE_X, 3, 4, 1}, 			// 3c
    {AND, "AND", ABSOLUTE_X, 3, 4, 1}, 			// 3d
    {ROL, "ROL", ABSOLUTE_X, 3, 7, 0}, 			// 3e
    {RLA, "RLA", ABSOLUTE_X, 0, 7, 0}, 			// 3f
    {RTI, "RTI", IMPLIED, 1, 6, 0}, 			// 40
    {EOR, "EOR", INDEXED_INDIRECT, 2, 6, 0}, 	// 41
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// 42
    {SRE, "SRE", INDEXED_INDIRECT, 0, 8, 0}, 	// 43
    {NOP, "NOP", ZEROPAGE, 2, 3, 0}, 			// 44
    {EOR, "EOR", ZEROPAGE, 2, 3, 0}, 			// 45
    {LSR, "LSR", ZEROPAGE, 2, 5, 0}, 			// 46
    {SRE, "SRE", ZEROPAGE, 0, 5, 0}, 			// 47
    {PHA, "PHA", IMPLIED, 1, 3, 0}, 			// 48
    {EOR, "EOR", IMMEDIATE, 2, 2, 0}, 			// 49
    {LSR, "LSR", ACCUMULATOR, 1, 2, 0},         // 4a
    {ALR, "ALR", IMMEDIATE, 0, 2, 0}, 			// 4b
    {JMP, "JMP", ABSOLUTE, 3, 3, 0}, 			// 4c
    {EOR, "EOR", ABSOLUTE, 3, 4, 0}, 			// 4d
    {LSR, "LSR", ABSOLUTE, 3, 6, 0}, 			// 4e
    {SRE, "SRE", ABSOLUTE, 0, 6, 0}, 			// 4f
    {BVC, "BVC", RELATIVE, 2, 2, 1}, 			// 50
    {EOR, "EOR", INDIRECT_INDEXED, 2, 5, 1}, 	// 51
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// 52
    {SRE, "SRE", INDIRECT_INDEXED, 0, 8, 0}, 	// 53
    {NOP, "NOP", ZEROPAGE_X, 2, 4, 0}, 			// 54
    {EOR, "EOR", ZEROPAGE_X, 2, 4, 0}, 			// 55
    {LSR, "LSR", ZEROPAGE_X, 2, 6, 0}, 			// 56
    {SRE, "SRE", ZEROPAGE_X, 0, 6, 0}, 			// 57
    {CLI, "CLI", IMPLIED, 1, 2, 0}, 			// 58
    {EOR, "EOR", ABSOLUTE_Y, 3, 4, 1}, 			// 59
    {NOP, "NOP", IMPLIED, 1, 2, 0}, 			// 5a
    {SRE, "SRE", ABSOLUTE_Y, 0, 7, 0}, 			// 5b
    {NOP, "NOP", ABSOLUTE_X, 3, 4, 1}, 			// 5c
    {EOR, "EOR", ABSOLUTE_X, 3, 4, 1}, 			// 5d
    {LSR, "LSR", ABSOLUTE_X, 3, 7, 0}, 			// 5e
    {SRE, "SRE", ABSOLUTE_X, 0, 7, 0}, 			// 5f
    {RTS, "RTS", IMPLIED, 1, 6, 0}, 			// 60
    {ADC, "ADC", INDEXED_INDIRECT, 2, 6, 0}, 	// 61
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// 62
    {RRA, "RRA", INDEXED_INDIRECT, 0, 8, 0}, 	// 63
    {NOP, "NOP", ZEROPAGE, 2, 3, 0}, 			// 64
    {ADC, "ADC", ZEROPAGE, 2, 3, 0}, 			// 65
    {ROR, "ROR", ZEROPAGE, 2, 5, 0}, 			// 66
    {RRA, "RRA", ZEROPAGE, 0, 5, 0}, 			// 67
    {PLA, "PLA", IMPLIED, 1, 4, 0}, 			// 68
    {ADC, "ADC", IMMEDIATE, 2, 2, 0}, 			// 69
    {ROR, "ROR", ACCUMULATOR, 1, 2, 0}, 		// 6a
    {ARR, "ARR", IMMEDIATE, 0, 2, 0}, 			// 6b
    {JMP, "JMP", INDIRECT, 3, 5, 0}, 			// 6c
    {ADC, "ADC", ABSOLUTE, 3, 4, 0}, 			// 6d
    {ROR, "ROR", ABSOLUTE, 3, 6, 0}, 			// 6e
    {RRA, "RRA", ABSOLUTE, 0, 6, 0}, 			// 6f
    {BVS, "BVS", RELATIVE, 2, 2, 1}, 			// 70
    {ADC, "ADC", INDIRECT_INDEXED, 2, 5, 1}, 	// 71
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// 72
    {RRA, "RRA", INDIRECT_INDEXED, 0, 8, 0}, 	// 73
    {NOP, "NOP", ZEROPAGE_X, 2, 4, 0}, 			// 74
    {ADC, "ADC", ZEROPAGE_X, 2, 4, 0}, 			// 75
    {ROR, "ROR", ZEROPAGE_X, 2, 6, 0}, 			// 76
    {RRA, "RRA", ZEROPAGE_X, 0, 6, 0}, 			// 77
    {SEI, "SEI", IMPLIED, 1, 2, 0}, 			// 78
    {ADC, "ADC", ABSOLUTE_Y, 3, 4, 1}, 			// 79
    {NOP, "NOP", IMPLIED, 1, 2, 0}, 			// 7a
    {RRA, "RRA", ABSOLUTE_Y, 0, 7, 0}, 			// 7b
    {NOP, "NOP", ABSOLUTE_X, 3, 4, 1}, 			// 7c
    {ADC, "ADC", ABSOLUTE_X, 3, 4, 1}, 			// 7d
    {ROR, "ROR", ABSOLUTE_X, 3, 7, 0}, 			// 7e
    {RRA, "RRA", ABSOLUTE_X, 0, 7, 0}, 			// 7f
    {NOP, "NOP", IMMEDIATE, 2, 2, 0}, 			// 80
    {STA, "STA", INDEXED_INDIRECT, 2, 6, 0}, 	// 81
    {NOP, "NOP", IMMEDIATE, 0, 2, 0}, 			// 82
    {SAX, "SAX", INDEXED_INDIRECT, 0, 6, 0}, 	// 83
    {STY, "STY", ZEROPAGE, 2, 3, 0}, 			// 84
    {STA, "STA", ZEROPAGE, 2, 3, 0}, 			// 85
    {STX, "STX", ZEROPAGE, 2, 3, 0}, 			// 86
    {SAX, "SAX", ZEROPAGE, 0, 3, 0}, 			// 87
    {DEY, "DEY", IMPLIED, 1, 2, 0}, 			// 88
    {NOP, "NOP", IMMEDIATE, 0, 2, 0}, 			// 89
    {TXA, "TXA", IMPLIED, 1, 2, 0}, 			// 8a
    {XAA, "XAA", IMMEDIATE, 0, 2, 0}, 			// 8b
    {STY, "STY", ABSOLUTE, 3, 4, 0}, 			// 8c
    {STA, "STA", ABSOLUTE, 3, 4, 0}, 			// 8d
    {STX, "STX", ABSOLUTE, 3, 4, 0}, 			// 8e
    {SAX, "SAX", ABSOLUTE, 0, 4, 0}, 			// 8f
    {BCC, "BCC", RELATIVE, 2, 2, 1}, 			// 90
    {STA, "STA", INDIRECT_INDEXED, 2, 6, 0}, 	// 91
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// 92
    {AHX, "AHX", INDIRECT_INDEXED, 0, 6, 0}, 	// 93
    {STY, "STY", ZEROPAGE_X, 2, 4, 0}, 			// 94
    {STA, "STA", ZEROPAGE_X, 2, 4, 0}, 			// 95
    {STX, "STX", ZEROPAGE_Y, 2, 4, 0}, 			// 96
    {SAX, "SAX", ZEROPAGE_Y, 0, 4, 0}, 			// 97
    {TYA, "TYA", IMPLIED, 1, 2, 0}, 			// 98
    {STA, "STA", ABSOLUTE_Y, 3, 5, 0}, 			// 99
    {TXS, "TXS", IMPLIED, 1, 2, 0}, 			// 9a
    {TAS, "TAS", ABSOLUTE_Y, 0, 5, 0}, 			// 9b
    {SHY, "SHY", ABSOLUTE_X, 0, 5, 0}, 			// 9c
    {STA, "STA", ABSOLUTE_X, 3, 5, 0}, 			// 9d
    {SHX, "SHX", ABSOLUTE_Y, 0, 5, 0}, 			// 9e
    {AHX, "AHX", ABSOLUTE_Y, 0, 5, 0}, 			// 9f
    {LDY, "LDY", IMMEDIATE, 2, 2, 0}, 			// a0
    {LDA, "LDA", INDEXED_INDIRECT, 2, 6, 0}, 	// a1
    {LDX, "LDX", IMMEDIATE, 2, 2, 0}, 			// a2
    {LAX, "LAX", INDEXED_INDIRECT, 0, 6, 0}, 	// a3
    {LDY, "LDY", ZEROPAGE, 2, 3, 0}, 			// a4
    {LDA, "LDA", ZEROPAGE, 2, 3, 0}, 			// a5
    {LDX, "LDX", ZEROPAGE, 2, 3, 0}, 			// a6
    {LAX, "LAX", ZEROPAGE, 0, 3, 0}, 			// a7
    {TAY, "TAY", IMPLIED, 1, 2, 0}, 			// a8
    {LDA, "LDA", IMMEDIATE, 2, 2, 0}, 			// a9
    {TAX, "TAX", IMPLIED, 1, 2, 0}, 			// aa
    {LAX, "LAX", IMMEDIATE, 0, 2, 0}, 			// ab
    {LDY, "LDY", ABSOLUTE, 3, 4, 0}, 			// ac
    {LDA, "LDA", ABSOLUTE, 3, 4, 0}, 			// ad
    {LDX, "LDX", ABSOLUTE, 3, 4, 0}, 			// ae
    {LAX, "LAX", ABSOLUTE, 0, 4, 0}, 			// af
    {BCS, "BCS", RELATIVE, 2, 2, 1}, 			// b0
    {LDA, "LDA", INDIRECT_INDEXED, 2, 5, 1}, 	// b1
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// b2
    {LAX, "LAX", INDIRECT_INDEXED, 0, 5, 1}, 	// b3
    {LDY, "LDY", ZEROPAGE_X, 2, 4, 0}, 			// b4
    {LDA, "LDA", ZEROPAGE_X, 2, 4, 0}, 			// b5
    {LDX, "LDX", ZEROPAGE_Y, 2, 4, 0}, 			// b6
    {LAX, "LAX", ZEROPAGE_Y, 0, 4, 0}, 			// b7
    {CLV, "CLV", IMPLIED, 1, 2, 0}, 			// b8
    {LDA, "LDA", ABSOLUTE_Y, 3, 4, 1}, 			// b9
    {TSX, "TSX", IMPLIED, 1, 2, 0}, 			// ba
    {LAS, "LAS", ABSOLUTE_Y, 0, 4, 1}, 			// bb
    {LDY, "LDY", ABSOLUTE_X, 3, 4, 1}, 			// bc
    {LDA, "LDA", ABSOLUTE_X, 3, 4, 1}, 			// bd
    {LDX, "LDX", ABSOLUTE_Y, 3, 4, 1}, 			// be
    {LAX, "LAX", ABSOLUTE_Y, 0, 4, 1}, 			// bf
    {CPY, "CPY", IMMEDIATE, 2, 2, 0}, 			// c0
    {CMP, "CMP", INDEXED_INDIRECT, 2, 6, 0}, 	// c1
    {NOP, "NOP", IMMEDIATE, 0, 2, 0}, 			// c2
    {DCP, "DCP", INDEXED_INDIRECT, 0, 8, 0}, 	// c3
    {CPY, "CPY", ZEROPAGE, 2, 3, 0}, 			// c4
    {CMP, "CMP", ZEROPAGE, 2, 3, 0}, 			// c5
    {DEC, "DEC", ZEROPAGE, 2, 5, 0}, 			// c6
    {DCP, "DCP", ZEROPAGE, 0, 5, 0}, 			// c7
    {INY, "INY", IMPLIED, 1, 2, 0}, 			// c8
    {CMP, "CMP", IMMEDIATE, 2, 2, 0}, 			// c9
    {DEX, "DEX", IMPLIED, 1, 2, 0}, 			// ca
    {AXS, "AXS", IMMEDIATE, 0, 2, 0}, 			// cb
    {CPY, "CPY", ABSOLUTE, 3, 4, 0}, 			// cc
    {CMP, "CMP", ABSOLUTE, 3, 4, 0}, 			// cd
    {DEC, "DEC", ABSOLUTE, 3, 6, 0}, 			// ce
    {DCP, "DCP", ABSOLUTE, 0, 6, 0}, 			// cf
    {BNE, "BNE", RELATIVE, 2, 2, 1}, 			// d0
    {CMP, "CMP", INDIRECT_INDEXED, 2, 5, 1}, 	// d1
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// d2
    {DCP, "DCP", INDIRECT_INDEXED, 0, 8, 0}, 	// d3
    {NOP, "NOP", ZEROPAGE_X, 2, 4, 0}, 			// d4
    {CMP, "CMP", ZEROPAGE_X, 2, 4, 0}, 			// d5
    {DEC, "DEC", ZEROPAGE_X, 2, 6, 0}, 			// d6
    {DCP, "DCP", ZEROPAGE_X, 0, 6, 0}, 			// d7
    {CLD, "CLD", IMPLIED, 1, 2, 0}, 			// d8
    {CMP, "CMP", ABSOLUTE_Y, 3, 4, 1}, 			// d9
    {NOP, "NOP", IMPLIED, 1, 2, 0}, 			// da
    {DCP, "DCP", ABSOLUTE_Y, 0, 7, 0}, 			// db
    {NOP, "NOP", ABSOLUTE_X, 3, 4, 1}, 			// dc
    {CMP, "CMP", ABSOLUTE_X, 3, 4, 1}, 			// dd
    {DEC, "DEC", ABSOLUTE_X, 3, 7, 0}, 			// de
    {DCP, "DCP", ABSOLUTE_X, 0, 7, 0}, 			// df
    {CPX, "CPX", IMMEDIATE, 2, 2, 0}, 			// e0
    {SBC, "SBC", INDEXED_INDIRECT, 2, 6, 0}, 	// e1
    {NOP, "NOP", IMMEDIATE, 0, 2, 0}, 			// e2
    {ISC, "ISC", INDEXED_INDIRECT, 0, 8, 0}, 	// e3
    {CPX, "CPX", ZEROPAGE, 2, 3, 0}, 			// e4
    {SBC, "SBC", ZEROPAGE, 2, 3, 0}, 			// e5
    {INC, "INC", ZEROPAGE, 2, 5, 0}, 			// e6
    {ISC, "ISC", ZEROPAGE, 0, 5, 0}, 			// e7
    {INX, "INX", IMPLIED, 1, 2, 0}, 			// e8
    {SBC, "SBC", IMMEDIATE, 2, 2, 0}, 			// e9
    {NOP, "NOP", IMPLIED, 1, 2, 0}, 			// ea
    {SBC, "SBC", IMMEDIATE, 0, 2, 0}, 			// eb
    {CPX, "CPX", ABSOLUTE, 3, 4, 0}, 			// ec
    {SBC, "SBC", ABSOLUTE, 3, 4, 0}, 			// ed
    {INC, "INC", ABSOLUTE, 3, 6, 0}, 			// ee
    {ISC, "ISC", ABSOLUTE, 0, 6, 0}, 			// ef
    {BEQ, "BEQ", RELATIVE, 2, 2, 1}, 			// f0
    {SBC, "SBC", INDIRECT_INDEXED, 2, 5, 1}, 	// f1
    {KIL, "KIL", IMPLIED, 0, 2, 0}, 			// f2
    {ISC, "ISC", INDIRECT_INDEXED, 0, 8, 0}, 	// f3
    {NOP, "NOP", ZEROPAGE_X, 2, 4, 0}, 			// f4
    {SBC, "SBC", ZEROPAGE_X, 2, 4, 0}, 			// f5
    {INC, "INC", ZEROPAGE_X, 2, 6, 0}, 			// f6
    {ISC, "ISC", ZEROPAGE_X, 0, 6, 0}, 			// f7
    {SED, "SED", IMPLIED, 1, 2, 0}, 			// f8
    {SBC, "SBC", ABSOLUTE_Y, 3, 4, 1}, 			// f9
    {NOP, "NOP", IMPLIED, 1, 2, 0}, 			// fa
    {ISC, "ISC", ABSOLUTE_Y, 0, 7, 0}, 			// fb
    {NOP, "NOP", ABSOLUTE_X, 3, 4, 1}, 			// fc
    {SBC, "SBC", ABSOLUTE_X, 3, 4, 1}, 			// fd
    {INC, "INC", ABSOLUTE_X, 3, 7, 0}, 			// fe
    {ISC, "ISC", ABSOLUTE_X, 0, 7, 0}, 			// ff
};

const int MEM_SIZE = 2048;
byte ram[MEM_SIZE]; // memory - 64k available to 6502, only 2k actually in NES

bool page_crossed = false;

// keep track of clock cycles
uint64_t cpu_ticks = 0;
#define CPU_TICK(x) (cpu_ticks += x)

// number of cycles to stall
int stall = 0;

// instruction info tables


// functions

// for info on this, see https://wiki.nesdev.com/w/index.php/CPU_power_up_state
void cpu_reset() {
    // clear memory
    memset(&ram, MEM_SIZE, 0);
    // reset stall
    stall = 0;
    // reset registers
    registers.a = 0;
    registers.x = 0;
    registers.y = 0;
    registers.sp = STACK_START;
    registers.pc = ((word)read_memory(RESET_VECTOR, ABSOLUTE)) | (((word)read_memory(RESET_VECTOR + 1, ABSOLUTE)) << 8);
    // reset flags
    flags.c = false;
    flags.z = false;
    flags.i = true;
    flags.d = false;
    flags.b = false;
    flags.v = false;
    flags.n = false;
}

void debugPrint(instruction_info info, byte opcode, word data) {
    printf("%.4X  %.2X %.2X %.2X  %s $%.4X\t\t A:%.2X X:%.2X Y:%.2X P:%.2X SP:%.2X %llu\n", PC, opcode, data & 0xFF, (data & 0xFF00) >> 8, info.name, data, A, X, Y, S, SP, cpu_ticks);
}

void cpu_cycle() {
    if (stall > 0) {
        stall--;
        CPU_TICK(1);
        #ifdef DEBUG
        printf("Stalling for another %d cycles.", stall);
        #endif
        return;
    }
    
    byte opcode = read_memory(PC, ABSOLUTE);
    page_crossed = false;
    bool jumped = false;
    instruction_info info = instructions[opcode];
    word data = 0;  // could be 8 bit or 16 bit, if 16 high byte is just 0
    for (byte i = 1; i < info.length; i++) {
        data |= (read_memory(PC + i, ABSOLUTE) << ((i-1) * 8));  // low byte comes first
    }
    //printf("%d %.4X\n", info.length, data);
    
    //#ifdef DEBUG
    //debugPrint(info, opcode, data);
    //#endif
    if (cpu_ticks % 1000000 < 7) {
        printf("hit %lld ticks\n", cpu_ticks);
    }
    
    switch (info.instruction) {
        case ADC: // add memory to accumulator with carry
        {
            byte src = read_memory(data, info.mode);
            unsigned int uiresult = (unsigned int)(src + A + C);
            int siresult = (int)((char)src + (char)A + (char)C);
            A += (src + C);
            C = (uiresult > 0xFF); // set carry
            V = ((siresult > 127) || (siresult < -128));  // set overflow
            setZN(A);
            break;
        }
        case AND: // bitwise AND with accumulator
        {
            byte src = read_memory(data, info.mode);
            A &= src;
            setZN(A);
            break;
        }
        case ASL: // arithmetic shift left
        {
            byte src = info.mode == ACCUMULATOR ? A : read_memory(data, info.mode);
            C = src >> 7; // carry is set to 7th bit
            src <<= 1;
            setZN(src);
            if (info.mode == ACCUMULATOR) {
                A = src;
            } else {
                write_memory(data, info.mode, src);
            }
            break;
        }
        case BCC: // branch if carry clear
            if (!C) {
                PC = address_for_mode(data, info.mode);
                jumped = true;
            }
            break;
        case BCS: // branch if carry set
            if (C) {
                PC = address_for_mode(data, info.mode);
                jumped = true;
            }
            break;
        case BEQ: // branch on result zero
            if (Z) {
                PC = address_for_mode(data, info.mode);
                jumped = true;
            }
            break;
        case BIT: // bit test bits in memory with accumulator
        {
            byte src = read_memory(data, info.mode);
            V = (src >> 6) & 1;
            Z = ((src & A) == 0);
            N = ((src >> 7) == 1);
            break;
        }
        case BMI: // branch on result minus
            if (N) {
                PC = address_for_mode(data, info.mode);
                jumped = true;
            }
            break;
        case BNE: // branch on result not zero
            if (!Z) {
                PC = address_for_mode(data, info.mode);
                jumped = true;
            }
            break;
        case BPL: // branch on result plus
            if (!N) {
                PC = address_for_mode(data, info.mode);
                jumped = true;
            }
            break;
        case BRK: // force break
            PC += 2;
            // push pc to stack
            ram[(0x0100 | SP)] = (byte)(PC >> 8);
            SP--;
            ram[(0x0100 | SP)] = (byte)(PC & 0xFF);
            SP--;
            B = true; // http://nesdev.com/the%20'B'%20flag%20&%20BRK%20instruction.txt
            // push S to stack
            ram[(0x0100 | SP)] = S;
            SP--;
            B = false;
            // set PC to reset vector
            PC = ((word)read_memory(IRQ_BRK_VECTOR, ABSOLUTE)) | (((word)read_memory(IRQ_BRK_VECTOR, ABSOLUTE) + 1) << 8);
            
            jumped = true;
            break;
        case BVC: // branch on overflow clear
            if (!V) {
                PC = address_for_mode(data, info.mode);
                jumped = true;
            }
            break;
        case BVS: // branch on overflow set
            if (V) {
                PC = address_for_mode(data, info.mode);
                jumped = true;
            }
            break;
        case CLC:  // clear carry
            C = false;
            break;
        case CLD: // clear decimal
            D = false;
            break;
        case CLI: // clear interrupt
            I = false;
            break;
        case CLV: // clear overflow
            V = false;
            break;
        case CMP: // compare accumulator
        {
            byte src = read_memory(data, info.mode);
            C = (A >= src);
            setZN(A - src);
            break;
        }
        case CPX: // compare X register
        {
            byte src = read_memory(data, info.mode);
            C = (X >= src);
            setZN(X - src);
            break;
        }
        case CPY: // compare Y register
        {
            byte src = read_memory(data, info.mode);
            C = (Y >= src);
            setZN(Y - src);
            break;
        }
        case DEC: // decrement memory
        {
            byte src = read_memory(data, info.mode);
            src--;
            write_memory(data, info.mode, src);
            setZN((src));
            break;
        }
        case DEX: // decrement X
            X--;
            setZN(X);
            break;
        case DEY: // decrement Y
            Y--;
            setZN(Y);
            break;
        case EOR: // exclusive or memory with accumulator
            A ^= read_memory(data, info.mode);
            setZN(A);
            break;
        case INC: // increment memory
        {
            byte src = read_memory(data, info.mode);
            src++;
            write_memory(data, info.mode, src);
            setZN((src));
            break;
        }
        case INX: // increment x
            X++;
            setZN(X);
            break;
        case INY: // increment y
            Y++;
            setZN(Y);
            break;
        case JMP: // jump to new location
            PC = address_for_mode(data, info.mode);
            jumped = true;
            break;
        case JSR: // jump to subroutine
        {
            // push next instruction address onto stack
            PC += 2;
            // push next instruction to stack
            ram[(0x0100 | SP)] = (byte)(PC >> 8);
            SP--;
            ram[(0x0100 | SP)] = (byte)(PC & 0xFF);
            SP--;
            // jump to subroutine
            PC = address_for_mode(data, info.mode);
            jumped = true;
            break;
        }
        case LDA: // load accumulator with memory
            A = read_memory(data, info.mode);
            setZN(A);
            break;
        case LDX: // load X with memory
            X = read_memory(data, info.mode);
            setZN(X);
            break;
        case LDY: // load Y with memory
            Y = read_memory(data, info.mode);
            setZN(Y);
            break;
        case LSR: // logical shift right
        {
            byte src = info.mode == ACCUMULATOR ? A : read_memory(data, info.mode);
            C = src & 1; // carry is set to 0th bit
            src >>= 1;
            setZN(src);
            if (info.mode == ACCUMULATOR) {
                A = src;
            } else {
                write_memory(data, info.mode, src);
            }
            break;
        }
        case NOP: // no op
            break;
        case ORA: // or memory with accumulator
            A |= read_memory(data, info.mode);
            setZN(A);
            break;
        case PHA: // push accumulator
            ram[(0x0100 | SP)] = A;
            SP--;
            break;
        case PHP: // push status
            B = true; // http://nesdev.com/the%20'B'%20flag%20&%20BRK%20instruction.txt
            ram[(0x0100 | SP)] = S;
            SP--;
            B = false;
            break;
        case PLA: // pull accumulator
            SP++;
            A = ram[(0x0100 | SP)];
            setZN(A);
            break;
        case PLP: // pull status
        {
            SP++;
            byte temp = ram[(0x0100 | SP)];
            C = temp & 0b00000001;
            Z = temp & 0b00000010;
            I = temp & 0b00000100;
            D = temp & 0b00001000;
            B = false; // http://nesdev.com/the%20'B'%20flag%20&%20BRK%20instruction.txt
            V = temp & 0b01000000;
            N = temp & 0b10000000;
            break;
        }
        case ROL: // rotate one bit left
        {
            byte src = info.mode == ACCUMULATOR ? A : read_memory(data, info.mode);
            C = (src >> 7) & 1; // carry is set to 7th bit
            src = ((src << 1) | C);
            setZN(src);
            if (info.mode == ACCUMULATOR) {
                A = src;
            } else {
                write_memory(data, info.mode, src);
            }
            break;
        }
        case ROR: // rotate one bit right
        {
            byte src = info.mode == ACCUMULATOR ? A : read_memory(data, info.mode);
            C = (src & 1); // carry is set to 0th bit
            src = ((src >> 1) | (C << 7));
            setZN(src);
            if (info.mode == ACCUMULATOR) {
                A = src;
            } else {
                write_memory(data, info.mode, src);
            }
            break;
        }
        case RTI: // return from interrupt
        {
            // pull Status out
            SP++;
            byte temp = ram[(0x0100 | SP)];
            C = temp & 0b00000001;
            Z = temp & 0b00000010;
            I = temp & 0b00000100;
            D = temp & 0b00001000;
            B = false; //http://nesdev.com/the%20'B'%20flag%20&%20BRK%20instruction.txt
            V = temp & 0b01000000;
            N = temp & 0b10000000;
            // pull PC out
            SP++;
            byte lb = ram[(0x0100 | SP)];
            SP++;
            byte hb = ram[(0x0100 | SP)];
            PC = (((word)hb) << 8) |  lb;
            jumped = true;
            break;
        }
        case RTS: // return from subroutine
        {
            // pull PC out
            SP++;
            byte lb = ram[(0x0100 | SP)];
            SP++;
            byte hb = ram[(0x0100 | SP)];
            PC = (((((word)hb) << 8) |  lb) + 1); // it was -1 on the other side
            jumped = true;
            break;
        }
        case SBC: // subtract with carry
        {
            byte src = read_memory(data, info.mode);
            unsigned int uiresult = (unsigned int)(src - A - C);
            int siresult = (int)((char)src - (char)A - (char)C);
            A -= (src - C);
            C = (uiresult > 0xFF); // set carry
            V = (siresult > 127 || siresult < -128);  // set overflow
            setZN(A);
            break;
        }
        case SEC: // set carry
            C = true;
            break;
        case SED: // set decimal
            D = true;
            break;
        case SEI: // set interrupt
            I = true;
            break;
        case STA: // store accumulator
            write_memory(data, info.mode, A);
            break;
        case STX: // store X register
            write_memory(data, info.mode, X);
            break;
        case STY: // store Y register
            write_memory(data, info.mode, Y);
            break;
        case TAX: // transfer a to x
            X = A;
            break;
        case TAY: // transfer a to y
            Y = A;
            break;
        case TSX: // transfer stack pointer to x
            X = SP;
            break;
        case TXS: // transfer x to stack pointer
            SP = X;
            break;
        case TXA: // transfer x to a
            A = X;
            break;
        case TYA: // transfer y to a
            A = Y;
            break;
        default:
            break;
    }
    
    if (!jumped) {
        PC += info.length;
    }
    
    CPU_TICK(info.ticks);
    if (page_crossed) {
        CPU_TICK(info.pageTicks);
    }
}

static inline byte read_memory(word data, mem_mode mode) {
    if (mode == IMMEDIATE) {
        return (byte)data;
    }
    word address = address_for_mode(data, mode);
    
    // figure out based on memory map http://wiki.nesdev.com/w/index.php/CPU_memory_map
    if (address < 0x2000) { // main ram 2 KB up to 0800
        return ram[address % 0x0800]; // mirrors for the next 6 KB
    } else if (address <= 0x3FFF) { // 2000-2007 is PPU, up to 3FFF mirrors it every 8 bytes
        word temp = ((address % 8) | 0x2000); // get data from ppu register
        return read_ppu_register(temp);
    } else if (address <= 0x4017) { // APU and IO
        return 0; // TODO put in APU stuff
    } else if (address <= 0x401F) { // usually disabled APU & IO
        return 0;
    } else if (address < 0x6000) { // IO usually unused
        return 0;
    } else { // must be in range 0x6000 to 0xFFFF CARTRIDGE data
        return rom->readCartridge(address);
    }
}

static inline void write_memory(word data, mem_mode mode, byte value) {
    if (mode == IMMEDIATE) {
        ram[data] = value;
        return;
    }
    word address = address_for_mode(data, mode);
    
    // figure out based on memory map http://wiki.nesdev.com/w/index.php/CPU_memory_map
    if (address < 0x2000) { // main ram 2 KB up to 0800
        ram[address % 0x0800] = value; // mirrors for the next 6 KB
    } else if (address <= 0x3FFF) { // 2000-2007 is PPU, up to 3FFF mirrors it every 8 bytes
        word temp = ((address % 8) | 0x2000); // write data to ppu register
        write_ppu_register(temp, value);
        return;
    } else if (address <= 0x4017) { // APU and IO
        if (address == 0x4014) { // dma transfer of sprite data
            // we iteratively read incase there is a change from one type of memory to another
            word fromAddress = (value * 0x100); // this is the address to start copying from
            byte tempArray[SPR_RAM_SIZE]; // not the most efficient, but no direct ppu mem access here
            for (int i = 0; i < SPR_RAM_SIZE; i++) {
                tempArray[i] = read_memory((fromAddress + i), ABSOLUTE);
            }
            dma_transfer(tempArray);
            // stall for 512 cycles while this completes
            stall = 512;
        }
        return; // TODO put in APU stuff
    } else if (address <= 0x401F) { // usually disabled APU & IO
        return;
    } else if (address < 0x6000) { // IO usually unused
        return;
    } else { // must be in range /0x6000 to 0xFFFF CARTRIDGE data
        rom->writeCartridge(address, value);
    }
}

static inline word address_for_mode(word data, mem_mode mode) {
    word address = 0;
    switch (mode) {
        case ABSOLUTE:
            address = data;
            break;
        case ABSOLUTE_X:
            address = data + X;
            page_crossed = DIFFERENT_PAGES(address, (address - X));
            break;
        case ABSOLUTE_Y:
            address = data + Y;
            page_crossed = DIFFERENT_PAGES(address, (address - Y));
            break;
        case INDEXED_INDIRECT:
        {
            word ls = (word) ram[(data + X)];
            word ms = (word) ram[(data + X) + 1];
            address = (ms << 8) & ls;
            break;
        }
        case INDIRECT:
        {
            word ls = (word) ram[data];
            word ms = (word) ram[data + 1];
            address = (ms << 8) & ls;
            break;
        }
        case INDIRECT_INDEXED:
        {
            word ls = (word) ram[data];
            word ms = (word) ram[data + 1];
            address = (ms << 8) & ls;
            address += Y;
            page_crossed = DIFFERENT_PAGES(address, (address - Y));
            break;
        }
        case RELATIVE:
            address = (data < 0x80) ? (PC + 2 + data) : (PC + 2 + (data - 256)); // signed
            break;
        case ZEROPAGE:
            address = data;
            break;
        case ZEROPAGE_X:
            address = (byte) (((byte) data) + X);
            break;
        case ZEROPAGE_Y:
            address = (byte) (((byte) data) + Y);
            break;
        default:
            break;
    }
    return address;
}

static inline void setZN(byte value) {
    Z = (value == 0);
    N = (((char)value) < 0);
}

#ifdef TEST
void PC_Move(word address) {
    PC = address;
}
#endif
