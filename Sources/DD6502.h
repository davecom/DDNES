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
#include "DDROM.h"
#include "DDTypes.h"
#include "DDPPU.h"
#include "DDAPU.h"
#include "DDInput.h"

typedef enum {
    DUMMY,
    ABSOLUTE,
    ABSOLUTE_X,
    ABSOLUTE_Y,
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
void cpu_reset(void);
void cpu_cycle(void);
#ifdef TEST
void PC_Move(word address);
#endif


static inline byte read_memory(word data, mem_mode mode);
static inline void write_memory(word data, mem_mode mode, byte value);
static inline word address_for_mode(word data, mem_mode mode);
static inline void setZN(byte value);
void trigger_NMI(void);

#endif /* DD6502_h */
