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
#include "DDTypes.h"

byte read_apu_status(void);
void write_apu_register(word address, byte value);

#endif /* DDAPU_h */
