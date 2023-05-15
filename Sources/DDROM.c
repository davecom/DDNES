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
        // default size is 8K if size not specified
        if (rom->header.prgRamSize == 0) {
            rom->prgRam = calloc(PRG_RAM_BASE_UNIT_SIZE, 1);
        } else {
            rom->prgRam = calloc(rom->header.prgRamSize * PRG_RAM_BASE_UNIT_SIZE, 1);
        }
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
    //printf("Read %zu bytes from prg rom", amountRead);
    
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
    static bool neverRead = true;
    if (neverRead) { // initialize first time to default value
        rom->registerData = ((uint32_t)(0xC)) << 8;
        neverRead = false;
    }
    if (address < 0x2000) { // CHR Rom
        byte control = ((rom->registerData >> 8) & 0xFF);
        if (control & 16) { // check CHR Rom bank mode bit if in 4kb mode
            // 4kb mode
            if (address < 0x1000) { // chr bank 0
                uint32_t bank = ((rom->registerData >> 16) & 0xFF);
                uint32_t chrAddress = (bank * 0x1000) + address;
                return rom->chrRom[chrAddress];
            } else { // chr bank 1
                // get chr bank 1
                uint32_t bank = ((rom->registerData >> 24) & 0xFF);
                uint32_t chrAddress = (bank * 0x1000) + (address - 0x1000);
                return rom->chrRom[chrAddress];
            }
        } else { // 8 kb mode uses 4 bits of chr bank 0
            uint32_t bank = (((rom->registerData >> 16) & 0x1E) >> 1);
            uint32_t chrAddress = (bank * 0x2000) + address;
            return rom->chrRom[chrAddress];
        }
        return rom->chrRom[address];
    } else if (address >= 0x6000 && address < 0x8000) {
        return rom->prgRam[address - 0x6000];
    } else if (address >= 0x8000) { // PRG ROM
        byte control = ((rom->registerData >> 8) & 0xFF);
        byte bank_control = (control & 0xC) >> 2;
        uint32_t bank = ((rom->registerData >> 32) & 0x0F);
        switch (bank_control) {
            case 2:
            {
                if (address < 0xC000) { // fix first bank at 0x8000
                    return rom->prgRom[address - 0x8000];
                } else { // second bank at 0xC000 is variable
                    uint32_t prgAddress = (bank * 0x4000) + (address - 0xC000);
                    return rom->prgRom[prgAddress];
                }
            }
            case 3:
            {
                if (address < 0xC000) { // switch bank at 0x8000
                    uint32_t prgAddress = (bank * 0x4000) + (address - 0x8000);
                    return rom->prgRom[prgAddress];
                } else { // fix bank at 0xC000
                    uint32_t prgAddress = ((uint32_t)address - 0xC000);
                    prgAddress += (PRG_ROM_BASE_UNIT_SIZE * (uint32_t)(rom->header.prgRomSize - 1));
                    return rom->prgRom[prgAddress];
                }
            }
            default: // 0, 1 which are 32kb blob
            {
                // ignore low bit of bank
                uint32_t prgAddress = ((bank >> 1) * 0x4000) + (address - 0x8000);
                return rom->prgRom[prgAddress];
            }
        }
    } else {
        fprintf(stderr, "Tried to read from cartridge at invalid address %.4X!\n", address);
        return 0;
    }
}

void writeMapper1(word address, byte value) {
    // rom->registerData map
    // byte 0 (rightmost) will be original sent byte
    // byte 1 (second right most), will be control byte
    // byte 2 will be CHR bank 0
    // byte 3 will be CHR bank 1
    // byte 4 will be PRG bank
    if (address < 0x2000) {
        if (rom->hasCharacterRAM) {
            rom->chrRom[address] = value;
        } else {
            printf("Tried writing to character ram in non-character rom game at %.4X.", address);
            return; // for mapper 0, treat writing to CHR as a no-op
        }
    } else if (address >= 0x6000 && address < 0x8000) {
        rom->prgRam[address - 0x6000] = value;
    } else if (address >= 0x8000) {
        static byte shiftRegister = 0;
        static byte shiftCount = 0;
        if (value & 0b10000000) {
            shiftRegister = 0;
            shiftCount = 0;
            rom->registerData &= 0xFFFFFFFFFFFF00FF;
            rom->registerData |= (0xC << 8);
            return;
        } else {
            shiftRegister |= ((value & 1) << shiftCount);
            shiftCount++;
            if (shiftCount == 5) {
                rom->registerData = (rom->registerData & 0xFFFFFFFFFFFFFF00);
                rom->registerData |= shiftRegister;
                shiftCount = 0;
                shiftRegister = 0;
            } else {
                return; // don't write anything if wrong time
            }
        }
        if (address <= 0x9FFF) { // control
            // erase old data
            rom->registerData &= 0xFFFFFFFFFFFF00FF;
            // put new data in
            rom->registerData |= ((rom->registerData & 0xFF) << 8);
            // mirroing is first two bits
            byte mirroring = rom->registerData & 3;
            rom->verticalMirroring = (mirroring == 2);
        } else if (address <= 0xBFFF) { // CHR bank 0
            // erase old data
            rom->registerData &= 0xFFFFFFFFFF00FFFF;
            // put new data in
            rom->registerData |= ((rom->registerData & 0xFF) << 16);
        } else if (address <= 0xDFFF) { // CHR bank 1
            // erase old data
            rom->registerData &= 0xFFFFFFFF00FFFFFF;
            // put new data in
            rom->registerData |= ((rom->registerData & 0xFF) << 24);
        } else if (address <= 0xFFFF) { // PRG Bank
            // erase old data
            rom->registerData &= 0xFFFFFF00FFFFFFFF;
            // put new data in
            rom->registerData |= ((rom->registerData & 0xFF) << 32);
        }
    }
    else {
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

