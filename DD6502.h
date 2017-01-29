//
//  DD6502.h
//  DDNES
//
//  Created by David Kopec on 1/26/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#ifndef DD6502_h
#define DD6502_h

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef uint8_t byte;
typedef uint16_t word;

typedef enum {
    DUMMY,
    ABSOLUTE,
    ABSOLUTE_X,
    ASBOLUTE_Y,
    ACCUMULATOR,
    IMMEDIATE,
    IMPLIED,
    INDEXED_INDIRECT,
    INDIRECT,
    INDIRECT_INDEXED,
    RELATIVE,
    ZEROPAGE,
    ZEROPAGE_X,
    ZEROPAGE_Y
} mem_mode;

/**
 Reset all registers, flags, and memory to defaults
 */
void cpu_reset();

void cpu_cycle();

byte read_memory(word address, mem_mode mode);
void write_memory(word address, mem_mode mode, byte value);

#endif /* DD6502_h */
