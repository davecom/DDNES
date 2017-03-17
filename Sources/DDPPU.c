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
            break;
        default:
            break;
    }
}

byte read_ppu_register(word address) {
    return 0;
}

// started from write to $4014
// fill spr_ram starting from cpuMemLocation
void dma_transfer(byte *cpuMemLocation) {
    memcpy(cpuMemLocation, spr_ram, SPR_RAM_SIZE);
}
