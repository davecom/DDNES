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

typedef uint8_t byte;
typedef uint16_t word;

uint64_t cpu_cycles = 0;
#define CPU_CYCLE(x) (cpu_cycles += x)



void cpu_reset();
byte read_memory(word address, );
void write_memory(word address, memory);

#endif /* DD6502_h */
