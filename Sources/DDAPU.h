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
<<<<<<< HEAD
<<<<<<< HEAD
#include "DDUI.h"
=======
// Apple OSX and iOS (Darwin)
#if defined(__APPLE__) && defined(__MACH__)
>>>>>>> 043f363506922437640979ae94155b84901c9d4e
=======
// Apple OSX and iOS (Darwin)
#if defined(__APPLE__) && defined(__MACH__)
>>>>>>> 043f363506922437640979ae94155b84901c9d4e
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
