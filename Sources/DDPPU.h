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

#define SPR_RAM_SIZE 256

void write_ppu_register(word address, byte value);
byte read_ppu_register(word address);
void dma_transfer(byte *cpuMemLocation);

#endif /* DDPPU_h */
