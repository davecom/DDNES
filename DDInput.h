//
//  DDInput.h
//  DDNES
//
//  Created by David Kopec on 1/19/19.
//  Copyright © 2019 David Kopec. All rights reserved.
//

#ifndef DDInput_h
#define DDInput_h

#include "DDTypes.h"

extern byte JOYPAD1;
extern byte JOYPAD2;

#define SETP1A JOYPAD1 |= 0x80
#define UNSETP1A JOYPAD1 &= 0x7F
#define SETP1B JOYPAD1 |= 0x40
#define UNSETP1B JOYPAD1 &= 0xBF
#define SETP1SELECT JOYPAD1 |= 0x20
#define UNSETP1SELECT JOYPAD1 &= 0xDF
#define SETP1START JOYPAD1 |= 0x10
#define UNSETP1START JOYPAD1 &= 0xEF

#endif /* DDInput_h */
