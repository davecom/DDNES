//
//  DDPPU.h
//  DDNES
//
//  Created by David Kopec on 3/1/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#ifndef DDPPU_h
#define DDPPU_h

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "DDTypes.h"
#include "DDROM.h"

#define SPR_RAM_SIZE 256
#define NAMETABLE_SIZE 2048
#define PALETTE_SIZE 32

void ppu_reset();
void write_ppu_register(word address, byte value);
byte read_ppu_register(word address);
static inline byte ppu_mem_read(word address);
static inline void ppu_mem_write(word address, byte value);
void dma_transfer(byte *cpuMemLocation);

#endif /* DDPPU_h */