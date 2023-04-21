//
//  DDROM.c
//  DDNES
//
//  Created by David Kopec on 2/9/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include "DDROM.h"

#define INES_MARKER 0x1A53454E
#define TRAINER_SIZE 512
#define PRG_ROM_BASE_UNIT_SIZE 16384
#define CHR_ROM_BASE_UNIT_SIZE 8192
#define PRG_RAM_BASE_UNIT_SIZE 8192

ines_rom *rom = NULL;

byte (*mapperReaders[3])(word address)= {
    readMapper0,
    readMapper1,
    readMapper2
};

void (*mapperWriters[3])(word address, byte value)= {
    writeMapper0,
    writeMapper1,
    writeMapper2
};

// returns false on failure
bool loadROM(const char *filePath) {
    FILE *file = fopen(filePath, "rb");
    if (file == NULL) { // couldn't open file
        fprintf(stderr, "Can't open output file %s!\n", filePath);
        return false;
    }
    
    rom = calloc(1, sizeof(ines_rom));
    //memset(&(rom->header), 0, sizeof(ines_header));
    //printf("ines_header size %d\n", sizeof(ines_header));
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
    
    // allocate trainer and copy from file
    if (rom->trainerExists) {
        rom->trainer = calloc(TRAINER_SIZE, sizeof(byte));
        amountRead = fread(rom->trainer, 1, TRAINER_SIZE, file);
        if (amountRead != TRAINER_SIZE) {
            fprintf(stderr, "File %s doesn't have enough data to fill trainer!\n", filePath);
            fclose(file); // clean up
            return false;
        }
    } else {
        rom->trainer = NULL;
    }
    
    // allocate battery backed ram memory
    if (rom->batteryBackedRAM) {
        rom->prgRam = calloc(rom->header.prgRamSize * PRG_RAM_BASE_UNIT_SIZE, 1);
    } else {
        rom->prgRam = NULL;
    }
    
    // allocate rom memory and copy from file
    rom->prgRom = calloc((int)rom->header.prgRomSize * PRG_ROM_BASE_UNIT_SIZE, 1);
    amountRead = fread(rom->prgRom, 1, (int)rom->header.prgRomSize * PRG_ROM_BASE_UNIT_SIZE, file);
    if (amountRead != (rom->header.prgRomSize * PRG_ROM_BASE_UNIT_SIZE)) {
        fprintf(stderr, "File %s doesn't have enough data to fill program ROM!\n", filePath);
        fclose(file); // clean up
        return false;
    }
    
    // allocate chr rom data and copy from file
    if(rom->header.chrRomSize > 0) {
        rom->chrRom = calloc(rom->header.chrRomSize * CHR_ROM_BASE_UNIT_SIZE, 1);
        amountRead = fread(rom->chrRom, 1, rom->header.chrRomSize * CHR_ROM_BASE_UNIT_SIZE, file);
        if (amountRead != (rom->header.chrRomSize * CHR_ROM_BASE_UNIT_SIZE)) {
            fprintf(stderr, "File %s doesn't have enough data to fill CHR ROM!\n", filePath);
            //fclose(file); // clean up
            //return false;
        }
        rom->hasCharacterRAM = false;
    } else { // if chrRomSize is 0 that means this uses chr_ram which is 8k
        rom->chrRom = calloc(CHR_ROM_BASE_UNIT_SIZE, 1);
        rom->hasCharacterRAM = true;
    }
    
    // get mapper version
    rom->mapper = ((rom->header.flags6 & 0xF0) >> 4) | (rom->header.flags7 & 0xF0);
    // Early roms ignored bytes 7-15 of the iNES header and put in the strange words
    // DiskDude!; this results in 64 being added to the mapper number
    if (memcmp("DiskDude!", (const char*)rom + 7, 9) == 0) { // early rom that ignored bytes 7-15 of header
        rom->mapper = rom->mapper - 64;
    }
    
    // setup mapper read/write methods
    rom->readCartridge = mapperReaders[rom->mapper];
    rom->writeCartridge = mapperWriters[rom->mapper];
    
    fclose(file); // clean up
    return true;
}

void unloadROM(void) {
    if (rom != NULL) {
        if (rom->trainer != NULL) {
            free(rom->trainer);
        }
        if (rom->batteryBackedRAM) {
            free(rom->prgRam);
        }
        free(rom->prgRom);
        if (rom->header.chrRomSize > 0) {
            free(rom->chrRom);
        }
        free(rom);
        rom = NULL;
    }
}

byte readMapper0(word address) {
    if (address < 0x2000) {
        return rom->chrRom[address];
    } else if (address >= 0x6000 && address < 0x8000) {
        if (rom->batteryBackedRAM) {
            return rom->prgRam[address - 0x6000];
        } else {
            printf("Tried reading from battery backed RAM at %x, but there is none", address);
            return 0;
        }
    } else if (address >= 0x8000) {
        if (rom->header.prgRomSize > 1) {
            
            return rom->prgRom[address - 0x8000];
        } else {
            return rom->prgRom[(address - 0x8000) % PRG_ROM_BASE_UNIT_SIZE];
        }
    } else {
        fprintf(stderr, "Tried to read from cartridge at invalid address %.4X!\n", address);
        return 0;
    }
}

void writeMapper0(word address, byte value) {
    if (address < 0x2000) {
        if (rom->hasCharacterRAM) {
            rom->chrRom[address] = value;
        } else {
            printf("Tried writing to character ram in non-character rom game at %.4X.", address);
            return; // for mapper 0, treat writing to CHR as a no-op
        }
    } else if (address >= 0x6000 && address < 0x8000) {
        if (rom->batteryBackedRAM) {
            rom->prgRam[address - 0x6000] = value;
        } else {
            printf("Tried writing %c at battery backed RAM at %x, but there is none", value, address);
        }
        
    } else {
        fprintf(stderr, "Tried to write to cartridge at invalid address %.4X!\n", address);
    }
}

byte readMapper1(word address) {
    if (address >= 0x6000 && address < 0x8000) {
        return rom->prgRam[address - 0x6000];
    } else if (address >= 0x8000) {
        if (rom->header.prgRomSize > 1) {
            return rom->prgRom[address - 0x8000];
        } else {
            return rom->prgRom[(address - 0x8000) % PRG_ROM_BASE_UNIT_SIZE];
        }
    } else {
        fprintf(stderr, "Tried to read from cartridge at invalid address %.4X!\n", address);
        return 0;
    }
}

void writeMapper1(word address, byte value) {
    if (address >= 0x6000 && address < 0x8000) {
        rom->prgRam[address - 0x6000] = value;
    } else {
        fprintf(stderr, "Tried to write to cartridge at invalid address %.4X!\n", address);
    }
}

byte readMapper2(word address) {
    if (address < 0x2000) {
        return rom->chrRom[address];
    } else if (address >= 0x8000 && address < 0xC000) { // switchable PRG ROM bank
        uint32_t prgAddress = (uint32_t)address - 0x8000;
        byte bank = ((rom->registerData & 0xF) % rom->header.prgRomSize);
        prgAddress += (PRG_ROM_BASE_UNIT_SIZE * (uint32_t)bank);
        return rom->prgRom[prgAddress];
    } else if (address >= 0xC000) { // PRG ROM bank fixed to the last bank
        uint32_t prgAddress = ((uint32_t)address - 0xC000);
        prgAddress += (PRG_ROM_BASE_UNIT_SIZE * (uint32_t)(rom->header.prgRomSize - 1));
        return rom->prgRom[prgAddress];
    } else {
        fprintf(stderr, "Tried to read from cartridge at invalid address %.4X!\n", address);
        return 0;
    }
}

void writeMapper2(word address, byte value) {
    if (address < 0x2000) {
        if (rom->hasCharacterRAM) {
            rom->chrRom[address] = value;
        } else {
            printf("Tried writing to character ram in non-character rom game at %.4X.", address);
            return; // for mapper 0, treat writing to CHR as a no-op
        }
    } else if (address >= 0x8000) {
        rom->registerData = value;
    } else {
        fprintf(stderr, "Tried to write to cartridge at invalid address %.4X!\n", address);
    }
}

