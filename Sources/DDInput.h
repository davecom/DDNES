//
//  DDInput.h
//  DDNES
//
//  Created by David Kopec on 1/19/19.
//  Copyright © 2019 David Kopec. All rights reserved.
//

#ifndef DDInput_h
#define DDInput_h

#include <stdbool.h>
#include "DDTypes.h"

typedef struct {
    bool a;
    bool b;
    bool select;
    bool start;
    bool up;
    bool down;
    bool left;
    bool right;
    bool strobe;
    long read_count;
} Joypad;

extern Joypad joypad1;
extern Joypad joypad2;

#endif /* DDInput_h */
