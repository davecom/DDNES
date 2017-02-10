//
//  DDROM.c
//  DDNES
//
//  Created by David Kopec on 2/9/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include "DDROM.h"

#define INES_MARKER 0x1A53454E

ines_rom * loadROM(char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (file == NULL) { // couldn't open file
        fprintf(stderr, "Can't open output file %s!\n", filePath);
        return NULL;
    }
    
    ines_rom *rom = calloc(1, sizeof(ines_rom));
    size_t amountRead = fread(&(rom->header), sizeof(ines_header), 1, file);
    //printf("%d header size\n", sizeof(ines_header));
    //printf("%d amountRead\n", amountRead);
    if (amountRead != 1) { // did we read one header?
        fprintf(stderr, "File %s doesn't have enough data to fill header!\n", filePath);
        fclose(file); // clean up
        return NULL;
    }
    // check this is actually an ines formatted rom by signature
    //printf("%x header marker\n", rom->header.marker);
    if (rom->header.marker != INES_MARKER) {
        fprintf(stderr, "File %s doesn't have a valid iNES marker!\n", filePath);
        fclose(file); // clean up
        return NULL;
    }
    fclose(file); // clean up
    return rom;
}
