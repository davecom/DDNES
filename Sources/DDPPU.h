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
#include <stdbool.h>
#include "DDTypes.h"


static inline void write_ppu_register(word address, byte value);
static inline byte read_ppu_register(word address);

#endif /* DDPPU_h */
