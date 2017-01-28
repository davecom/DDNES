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
    IMMEDIATE,
    ZERO_PAGE,
    ZERO_PAGE_X,
    ABSOLUTE,
    ABSOLUTE_X,
    ASBOLUTE_Y,
    INDIRECT_X,
    INDIRECT_Y
} mem_mode;

/**
 Reset all registers, flags, and memory to defaults
 */
void cpu_reset();

void cpu_cycle();

byte read_memory(word address, mem_mode mode);
void write_memory(word address, mem_mode mode, byte value);

#endif /* DD6502_h */
