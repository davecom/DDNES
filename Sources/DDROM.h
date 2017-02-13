//
//  DDROM.h
//  DDNES
//
//  Created by David Kopec on 2/9/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#ifndef DDROM_h
#define DDROM_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "DDTypes.h"

typedef struct {
    uint32_t marker;
    byte prgRomSize; // in 16 kb units
    byte chrRomSize; // in 8 kb units
    byte flags6;
    byte flags7;
    byte prgRamSize; // in 8 kb units
    byte flags9;
    byte flags10;
    byte zeros[5];
} ines_header;

typedef struct {
    ines_header header;
    byte *trainer;
    byte *prgRom;
    byte *chrRom;
    byte *prgRam;
    byte *playChoiceInstRom;
    byte *playChoiceProm;
    byte mapper;
    bool verticalMirroring;
    bool batteryBackedRAM;
    bool trainerExists;
    bool fourScreenVRAM;
    bool PAL;
    bool vsSystem;
    byte (*readCartridge)(word address);
    void (*writeCartridge)(word address, byte value);
} ines_rom;

extern ines_rom *rom;

bool loadROM(char *filePath);
void unloadROM();

byte readMapper0(word address);
void writeMapper0(word address, byte value);

byte readMapper1(word address);
void writeMapper1(word address, byte value);

#endif /* DDROM_h */
