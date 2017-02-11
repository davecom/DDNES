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

typedef struct {
    uint32_t marker;
    uint8_t prgRomSize; // in 16 kb units
    uint8_t chrRomSize; // in 8 kb units
    uint8_t flags6;
    uint8_t flags7;
    uint8_t prgRamSize; // in 8 kb units
    uint8_t flags9;
    uint8_t flags10;
    uint8_t zeros[5];
} ines_header;

typedef struct {
    ines_header header;
    uint8_t *trainer;
    uint8_t *prgRom;
    uint8_t *chrRom;
    uint8_t *playChoiceInstRom;
    uint8_t *playChoiceProm;
    uint8_t mapper;
    bool verticalMirroring;
    bool batteryBackedRAM;
    bool trainerExists;
    bool fourScreenVRAM;
    bool PAL;
    bool vsSystem;
} ines_rom;

bool loadROM(char *filePath);

#endif /* DDROM_h */
