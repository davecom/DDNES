//
//  DDROM.c
//  DDNES
//
//  Created by David Kopec on 2/9/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include "DDROM.h"

#define INES_MARKER 0x1A53454E

ines_rom *rom = NULL;

// returns false on failure
bool loadROM(char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (file == NULL) { // couldn't open file
        fprintf(stderr, "Can't open output file %s!\n", filePath);
        return false;
    }
    
    rom = calloc(1, sizeof(ines_rom));
    size_t amountRead = fread(&(rom->header), sizeof(ines_header), 1, file);
    //printf("%d header size\n", sizeof(ines_header));
    //printf("%d amountRead\n", amountRead);
    if (amountRead != 1) { // did we read one header?
        fprintf(stderr, "File %s doesn't have enough data to fill header!\n", filePath);
        fclose(file); // clean up
        return false;
    }
    //printf("%d rom size\n", rom->header.prgRomSize);
    // check this is actually an ines formatted rom by signature
    //printf("%x header marker\n", rom->header.marker);
    if (rom->header.marker != INES_MARKER) {
        fprintf(stderr, "File %s doesn't have a valid iNES marker!\n", filePath);
        fclose(file); // clean up
        return false;
    }
    
    rom->verticalMirroring = ((rom->header.flags6 & 0b00000001) > 0);
    rom->batteryBackedRAM = ((rom->header.flags6 & 0b00000010) > 0);
    rom->trainerExists = ((rom->header.flags6 & 0b00000100) > 0);
    rom->fourScreenVRAM = ((rom->header.flags6 & 0b00001000) > 0);
    rom->vsSystem = ((rom->header.flags7 & 0b00000001) > 0);
    rom->PAL = ((rom->header.flags9 & 0b00000001) > 0);
    
    // get mapper version
    rom->mapper = (rom->header.flags6 & 0xF0 >> 4) | (rom->header.flags7 & 0xF0);
    
    fclose(file); // clean up
    return true;
}
