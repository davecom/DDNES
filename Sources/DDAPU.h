//
//  DDAPU.h
//  DDNES
//
//  Created by David Kopec on 11/10/19.
//  Copyright © 2019 David Kopec. All rights reserved.
//

#ifndef DDAPU_h
#define DDAPU_h

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "DDTypes.h"
// Apple OSX and iOS (Darwin)
#if defined(__APPLE__) && defined(__MACH__)
#include <SDL2/SDL.h>
// Windows, Linux
#else
#include <SDL.h>
#endif

byte read_apu_status(void);
void write_apu_register(word address, byte value);
void apu_tick(void);
void apu_reset(void);

#endif /* DDAPU_h */
