//
//  DDPPU.c
//  DDNES
//
//  Created by David Kopec on 3/1/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include "DDPPU.h"

struct {
    byte ppu_control1; // PPU Control Register 1 $2000
    byte ppu_control2; // PPU Control Register 2 $2001
    byte ppu_status; // PPU Status Register $2002
    byte spr_ram_address; // SPR-RAM address to write next to through $2003
    byte spr_ram_io; // SPR-RAM I/O register (what to write to $2003) $2004
    // based on http://wiki.nesdev.com/w/index.php/PPU_scrolling
    word v; // current VRAM address (15 bits)
    word t; // temporary VRAM address (15 bits)
    byte x; // fine X scroll (3 bits)
    bool w; // write toggle
} registers;

#define PPU_CONTROL1 registers.ppu_control1
#define PPU_CONTROL2 registers.ppu_control2
#define PPU_STATUS registers.ppu_status
#define SPR_RAM_ADDRESS registers.spr_ram_address
#define SPR_RAM_IO registers.spr_ram_io
#define V registers.v
#define T registers.t
#define X registers.x
#define W registers.w

#define NAME_TABLE_ADDRESS (PPU_CONTROL1 & 0b00000011)
#define ADDRESS_INCREMENT ((PPU_CONTROL1 & 0b00000100) ? 32 : 1)

byte spr_ram[SPR_RAM_SIZE]; // sprite RAM
byte nametables[NAMETABLE_SIZE]; // nametable ram
byte palette[PALETTE_SIZE]; // pallete ram

// for info on this, see http://wiki.nesdev.com/w/index.php/PPU_power_up_state
void ppu_reset() {
    // clear memory
    memset(&spr_ram, SPR_RAM_SIZE, 0);
    memset(&nametables, NAMETABLE_SIZE, 0);
    memset(&palette, PALETTE_SIZE, 0);
    // reset registers
    PPU_CONTROL1 = 0;
    PPU_CONTROL2 = 0;
    PPU_STATUS = 0;
    SPR_RAM_ADDRESS = 0;
    SPR_RAM_IO = 0;
    V = 0;
    T = 0;
    X = 0;
    W = false;
}


void write_ppu_register(word address, byte value) {
    switch(address) {
        case 0x2000:
            PPU_CONTROL1 = value;
            break;
        case 0x2001:
            PPU_CONTROL2 = value;
            break;
        case 0x2003:
            SPR_RAM_ADDRESS = value;
            break;
        case 0x2004:
            SPR_RAM_IO = value;
            spr_ram[SPR_RAM_ADDRESS] = value;
            break;
        case 0x2005:  // based on http://wiki.nesdev.com/w/index.php/PPU_scrolling
            if (!W) { // first write
                T = (T & 0b1111111111100000) | (value >> 3);
                X = (value & 0b00000111);
            } else { // second write
                T = (T & 0b1000111111111111) | (((word)value & 0b00000111) << 12);
                T = (T & 0b1111110000011111) | (((word)value & 0b11111000) << 2);
            }
            W = !W;
            break;
        case 0x2006: // based on http://wiki.nesdev.com/w/index.php/PPU_scrolling
            if (!W) { // first write
                T = (T & 0b0000000011111111) | (((word)value & 0b00111111) << 8);
            } else { // second write
                T = (T & 0b1111111100000000) | value;
                V = T;
            }
            W = !W;
            break;
        case 0x2007: // implement writing to vram
            ppu_mem_write(V, value);
            break;
        default:
            fprintf(stderr, "ERROR: Unrecognized PPU register write %X", address);
            break;
    }
}

byte read_ppu_register(word address) {
    switch(address) {
        case 0x2002:
            return PPU_STATUS;
        case 0x2004:
            return spr_ram[SPR_RAM_ADDRESS];
        case 0x2007: // implement writing to vram
            return ppu_mem_read(V);
        default:
            fprintf(stderr, "ERROR: Unrecognized PPU register read %X", address);
            return 0;
    }
    
}

static inline byte ppu_mem_read(word address) {
    address = address % 0x4000; // mirror >0x4000 back to it
    if (address < 0x2000) { // pattern tables
        return rom->readCartridge(address);
    } else if (address < 0x3F00) { // name tables
        address = (address - 0x2000) % 0x1000;
        return nametables[address];
    } else if (address < 0x4000) {
        address = (address - 0x3F00) % 0x20;
        return palette[address];
    } else {
        fprintf(stderr, "ERROR: Unrecognized PPU address read %X", address);
        return 0;
    }
}

static inline void ppu_mem_write(word address, byte value) {
    address = address % 0x4000; // mirror >0x4000 back to it
    if (address < 0x2000) { // pattern tables
        rom->writeCartridge(address, value);
    } else if (address < 0x3F00) { // name tables
        address = (address - 0x2000) % 0x1000;
        nametables[address] = value;
    } else if (address < 0x4000) {
        address = (address - 0x3F00) % 0x20;
        palette[address] = value;
    } else {
        fprintf(stderr, "ERROR: Unrecognized PPU address write %X", address);
    }
}

// started from write to $4014
// fill spr_ram starting from cpuMemLocation
void dma_transfer(byte *cpuMemLocation) {
    memcpy(cpuMemLocation, spr_ram, SPR_RAM_SIZE);
}