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
    // based on http://wiki.nesdev.com/w/index.php/PPU_scrolling
    word v; // current VRAM address (15 bits)
    word t; // temporary VRAM address (15 bits)
    byte x; // fine X scroll (3 bits)
    bool w; // write toggle
} ppu_registers;

#define PPU_CONTROL1 ppu_registers.ppu_control1
#define PPU_CONTROL2 ppu_registers.ppu_control2
#define PPU_STATUS ppu_registers.ppu_status
#define SPR_RAM_ADDRESS ppu_registers.spr_ram_address
#define SPR_RAM_IO ppu_registers.spr_ram_io
#define V ppu_registers.v
#define T ppu_registers.t
#define X ppu_registers.x
#define W ppu_registers.w

#define NAME_TABLE_ADDRESS ((((word)PPU_CONTROL1) & 0b00000011) * 0x400)
#define ADDRESS_INCREMENT ((PPU_CONTROL1 & 0b00000100) ? 32 : 1)
// true if the sprite is 8x16
#define SPRITE_HEIGHT (PPU_CONTROL1 & 0b00100000)
// ignore for 8x16 sprites
#define SPRITE_PATTERN_TABLE_ADDRESS (SPRITE_HEIGHT ? 0 : (((((word)PPU_CONTROL1) & 0b00001000) >> 3) * 0x1000))
#define BACKGROUND_PATTERN_TABLE_ADDRESS (((((word)PPU_CONTROL1) & 0b00010000) >> 4) * 0x1000)

#define GENERATE_NMI (PPU_CONTROL1 & 0b10000000)

#define SHOW_BACKGROUND (PPU_CONTROL2 & 0b00001000)
#define SHOW_SPRITES (PPU_CONTROL2 & 0b00010000)
#define LEFT_8_SPRITE_SHOW (PPU_CONTROL2 & 0b00000100)
#define LEFT_8_BACKGROUND_SHOW (PPU_CONTROL2 & 0b00000010)
#define VBLANK (PPU_STATUS & 0b10000000)

byte spr_ram[SPR_RAM_SIZE]; // sprite RAM
byte secondary_spr_ram[SECONDARY_SPR_RAM_SIZE]; // secondary sprite RAM
byte nametables[NAMETABLE_SIZE]; // nametable ram
byte palette[PALETTE_SIZE]; // pallete ram

// for info on this, see http://wiki.nesdev.com/w/index.php/PPU_power_up_state
void ppu_reset(void) {
    // clear memory
    memset(spr_ram, 0, SPR_RAM_SIZE);
    memset(nametables, 0, NAMETABLE_SIZE);
    memset(palette, 0, PALETTE_SIZE);
    // reset registers
    PPU_CONTROL1 = 0;
    PPU_CONTROL2 = 0;
    PPU_STATUS = 0;
    SPR_RAM_ADDRESS = 0;
    V = 0;
    T = 0;
    X = 0;
    W = false;
}

byte scanline_sprite_count = 0;
bool sprite_zero_in_secondary = false;

static void draw_nametables(void) {
    for (int nametable = 0; nametable < 4; nametable++) {
        word base_nametable_address = 0x2000 + 0x400 * nametable;
        word base_attributetable_address = 0x2000 + 0x400 * nametable + 0x3C0;
        for (int y = 0; y < 30; y++) {
            for (int x = 0; x < 32; x++) {
                word tile_address = base_nametable_address + y * 0x20 + x;
                byte nametable_entry = ppu_mem_read(tile_address);
                int attrx = x / 4;
                int attry = y / 4;
                word attribute_address = base_attributetable_address + attry * 8 + attrx;
                byte attribute_entry = ppu_mem_read(attribute_address);
                int block = (y & 0x02) | ((x & 0x02) >> 1); // https://forums.nesdev.com/viewtopic.php?f=10&t=13315
                byte attribute_bits = 0;
                switch (block) {
                    case 0:
                        attribute_bits = (attribute_entry & 0b00000011) << 2;
                        break;
                    case 1:
                        attribute_bits = (attribute_entry & 0b00001100);
                        break;
                    case 2:
                        attribute_bits = (attribute_entry & 0b00110000) >> 2;
                        break;
                    case 3:
                        attribute_bits = (attribute_entry & 0b11000000) >> 4;
                        break;
                    default:
                        printf("Invalid block");
                }
                for (int fine_y = 0; fine_y < 8; fine_y++) {
                    byte low_order = ppu_mem_read(BACKGROUND_PATTERN_TABLE_ADDRESS +  nametable_entry * 16 + fine_y);
                    byte high_order = ppu_mem_read(BACKGROUND_PATTERN_TABLE_ADDRESS +  nametable_entry * 16 + 8 + fine_y);
                    for (int fine_x = 0; fine_x < 8; fine_x++) {
                        byte pixel = ((low_order >> (7 - fine_x)) & 1) | (((high_order >> (7 - fine_x)) & 1) << 1) | attribute_bits;
                        int x_screen_loc = x * 8 + fine_x + ((nametable % 2 == 0) ? 0 : 256);
                        int y_screen_loc = y * 8 + fine_y + ((nametable < 2) ? 0 : 240);
                        bool transparent_background = (pixel & 3) == 0;
                        // if the background is transparent, we use the first color
                        // in the palette
                        draw_nametables_pixel(x_screen_loc, y_screen_loc, transparent_background ? palette[0] : palette[pixel]);
                    }
                    
                }
            }
        }
    }
}

// figure out which sprites to draw and put them in secondary RAM
// assume 8 x 8 sprites for now
static void figure_out_sprites(int scanline) {
    sprite_zero_in_secondary = false;
    memset(secondary_spr_ram, 0xFF, SECONDARY_SPR_RAM_SIZE); // reset memory
    scanline_sprite_count = 0; // only take first 8 sprites
    for (int i = 0; i < SPR_RAM_SIZE; i += 4) {
        byte y_position = spr_ram[i];
        if (y_position == 0xFF) {
            continue; // 0xFF is marker for no sprite data
        }
        byte sprite_line = scanline - y_position; // where vertically in the sprite are we
        
        if (sprite_line >= 0 && sprite_line < (SPRITE_HEIGHT ? 16 : 8)) { // somewhere on this scanline
            // copy memory over to secondary ram for display readiness
            memcpy(secondary_spr_ram + (scanline_sprite_count * 4), spr_ram + i, 4);
            if (i == 0) {
                sprite_zero_in_secondary = true;
            }
            if (scanline_sprite_count >= 7) { // don't allow more than 8 sprites per scanline
                PPU_STATUS |= 0b00100000; // set sprite overflow bit
                break;
            }
            scanline_sprite_count++;
        }
        
        //printf("%d", spr_ram[i]);
        
        
    }
}

static void draw_sprite_pixel(int x, int y, bool background_transparent) {
    int scanline = y - 1;  // we actually draw sprites shifted one pixel down
    for (int i = scanline_sprite_count * 4; i >= 0; i -= 4) {
        byte y_position = secondary_spr_ram[i];
        if (y_position == 0xFF) {
            continue;
        }
        bool background_sprite = (secondary_spr_ram[i + 2] >> 5) & 1;
        byte x_position = secondary_spr_ram[i + 3];
        
        if (x >= x_position && x < (x_position + 8)) { // somewhere in this sprite
            bool flip_y = (secondary_spr_ram[i + 2] >> 7) & 1;
            byte sprite_line = scanline - y_position; // where vertically in the sprite are we
            if (flip_y) {
                sprite_line = (SPRITE_HEIGHT ? 15 : 7) - sprite_line;
            }
            byte index = secondary_spr_ram[i + 1];
            word bit0sAddress;
            if (SPRITE_HEIGHT) {
                bit0sAddress = ((index & 1) ? 0x1000 : 0) + ((index & 0xFE) * 16) + ((sprite_line & 7) | (((sprite_line > 7) ? 1 : 0) << 4));
            } else {
                bit0sAddress = SPRITE_PATTERN_TABLE_ADDRESS + (index * 16) + sprite_line;
            }
            word bit1sAddress = bit0sAddress + 8;
            byte bit0s = ppu_mem_read(bit0sAddress);
            byte bit1s = ppu_mem_read(bit1sAddress);
            byte bit3and2 = ((secondary_spr_ram[i + 2]) & 3) << 2;
            // draw the 8 pixels on this scanline
            bool flip_x = (secondary_spr_ram[i + 2] >> 6) & 1;
            
            byte x_loc = x - x_position; // position within sprite
            if (!flip_x) {
                x_loc = 7 - x_loc;
            }
            
            byte bit1and0 = (((bit1s >> x_loc) & 1) << 1) | (((bit0s >> x_loc) & 1) << 0);
            if (bit1and0 == 0) { // transparent pixel... skip
                continue;
            }
            // this is not transparent, is it a sprite zero hit therefore?
            // check left 8 pixel clipping is not off
            if (i == 0 && !background_transparent && sprite_zero_in_secondary && !(x < 8 && (!LEFT_8_SPRITE_SHOW || !LEFT_8_BACKGROUND_SHOW)) && SHOW_BACKGROUND && SHOW_SPRITES) {
                PPU_STATUS |= 0b01000000;
            }
            // need to do this after sprite zero checking so we still count background
            // sprites for sprite zero checks
            if (background_sprite && !background_transparent) {
                continue;  // don't draw over opaque background pixels if this is background sprite
            }
            
            byte color = bit3and2 | bit1and0;
            color = ppu_mem_read(0x3F10 + color); // pull from palette memory
            draw_pixel(x, y, color);
        }
    }
}


// Based on https://wiki.nesdev.com/w/index.php/PPU_scrolling
// and https://github.com/fogleman/nes/blob/master/nes/ppu.go
void ppu_step(void) {
    static int scanline = 0;
    static int cycle = 0;
    static byte name_table_byte = 0;
    static byte shift = 0;
    static byte attribute_table_byte = 0;
    static byte low_tile_byte = 0;
    static byte high_tile_byte = 0;
    static uint64_t tile_data = 0;
    static word address = 0; // temp;
    static byte fine_y = 0;
    uint32_t temp_data = 0;
    //printf("scanline %d cycle %d V %x\n", scanline, cycle, V);
    if (SHOW_BACKGROUND) {
        if (scanline < 240 && cycle > 0 && cycle <= 256) {
            
            // render
            byte color = ((tile_data >> 32) >> ((7 - X) * 4)) & 0x0F;
            bool transparent_background = (color & 3) == 0;
            // if the background is transparent, we use the first color
            // in the palette
            draw_pixel(cycle - 1, scanline, transparent_background ? palette[0] : palette[color]);
            if (SHOW_SPRITES) {
                draw_sprite_pixel(cycle - 1, scanline, transparent_background);
            }
        }
        if ((scanline < 240 || scanline == 261) && ((cycle > 0 && cycle <= 256) || (cycle >= 321 && cycle <= 336))) {
            
            // prepare for next render
            tile_data <<= 4;
            switch (cycle % 8) {
                case 1: // fetch name table byte (2 cycles)
                    address = 0x2000 | (V & 0x0FFF);
                    //printf("nt address is %x\n", address);
                    name_table_byte = ppu_mem_read(address);
                    break;
                case 3: // fetch attribute table byte (2 cycles)
                    address = 0x23C0 | (V & 0x0C00) | ((V >> 4) & 0x38) | ((V >> 2) & 0x07);
                    shift = ((V >> 4) & 4) | (V & 2); //? from Fogleman need to investigate more
                    //printf("at address is %x\n", address);
                    attribute_table_byte = ((ppu_mem_read(address) >> shift) &
                                            3) << 2; // this is upper 2 bits for all 8 pixels, so move into place for every pixel ahead of time
                    break;
                case 5:
                    fine_y = (V >> 12) & 7;
                    //printf("background pattern table address is %x\n", BACKGROUND_PATTERN_TABLE_ADDRESS);
                    //printf("name_table_byte is %x\n", name_table_byte);
                    //printf("fine_y is %x\n", fine_y);
                    address = BACKGROUND_PATTERN_TABLE_ADDRESS + ((word)name_table_byte) * 16 + fine_y;
                    //printf("ltb address is %x\n", address);
                    low_tile_byte = ppu_mem_read(address);
                    break;
                case 7:
                    fine_y = (V >> 12) & 7;
                    address = BACKGROUND_PATTERN_TABLE_ADDRESS + ((word)name_table_byte) * 16 + fine_y;
                    //printf("htb address is %x\n", address + 8);
                    high_tile_byte = ppu_mem_read(address + 8);
                    break;
                case 0:
                    for (int i = 7; i >= 0; i--) {
                        temp_data <<= 4;
                        
                        temp_data |= ((low_tile_byte & (1 << i)) >> i);
                        if (i > 0) {
                            temp_data |= ((high_tile_byte & (1 << i)) >> (i - 1));
                        } else {
                            temp_data |= ((high_tile_byte & (1 << i)) << 1);
                        }
                        temp_data |= attribute_table_byte;
                    }
                    tile_data |= temp_data;
                    if ((cycle >= 321 && cycle <= 336) || cycle <= 256) {
                        // coarse X scroll from
                        // based on http://wiki.nesdev.com/w/index.php/PPU_scrolling
                        
                        if ((V & 0x001F) == 31) { // if coarse X == 31
                            V &= ~0x001F;         // coarse X = 0
                            V ^= 0x0400;           // switch horizontal nametable
                        }
                        else {
                            V += 1;                // increment coarse X
                        }
                    }
                    
                    break;
            }
            
        }
        if ((scanline < 240 || scanline == 261) && cycle == 256) { // based on http://wiki.nesdev.com/w/index.php/PPU_scrolling
            // increment y
            if ((V & 0x7000) != 0x7000) {       // if fine Y < 7
                V += 0x1000;                     // increment fine Y
            } else {
                V &= ~0x7000;                     // fine Y = 0
                int y = (V & 0x03E0) >> 5;        // let y = coarse Y
                if (y == 29) {
                    y = 0;                          // coarse Y = 0
                    V ^= 0x0800;                    // switch vertical nametable
                } else if (y == 31) {
                    y = 0;                          // coarse Y = 0, nametable not switched
                } else {
                    y += 1;                         // increment coarse Y
                }
                V = (V & ~0x03E0) | (y << 5);     // put coarse Y back into v
            }
            
        }
        if ((scanline < 240 || scanline == 261) && cycle == 257) { // copy X scroll from T to V
            // copy x
            // based on http://wiki.nesdev.com/w/index.php/PPU_scrolling
            V = (V & 0xFBE0) | (T & 0x041F);
            
        }
        if (cycle == 257) {
            if (SHOW_SPRITES) {
                if (scanline < 240) {
                    figure_out_sprites(scanline);
                } else {
                    scanline_sprite_count = 0;
                }
            }
        }
        // clear PPU_STATUS ($2002 at dot 1 of the pre-render line
        if (scanline == 261 && cycle == 1) {
            PPU_STATUS &= 0b00011111;
            if (nametable_debug) {
                draw_nametables();
            }
        }
        
        if (scanline == 261 && cycle >= 280 && cycle <= 304) { // copy y
            V = (V & 0x841F) | (T & 0x7BE0);
        }
        
    }
    
    if (scanline == 241 && cycle == 1) {
        
        //frame_ready();
        PPU_STATUS |= 0b10000000; // set vblank
        if (GENERATE_NMI) {
            trigger_NMI(); // trigger NMI
        }
    }
    
    if (scanline == 261 && cycle == 1) {
        PPU_STATUS &= 0b00111111; // vblank off, clear sprite zero hit
    }
    

    
    cycle++;
    
    if (cycle > 340) {
        cycle = 0;
        
        scanline++;
        if (scanline > 261) {
            scanline = 0;
        }
    }
}


void write_ppu_register(word address, byte value) {
    switch(address) {
        case 0x2000:
            PPU_CONTROL1 = value;
            T = (T & 0xF3FF) | ((((word)value) & 0x03) << 10);
            // putting in the following immediate nmi generation broke Tennis
            // immediately generate nmi if called for
//            if (VBLANK && GENERATE_NMI) {
//                trigger_NMI();
//            }
            break;
        case 0x2001:
            PPU_CONTROL2 = value;
            break;
        case 0x2003:
            SPR_RAM_ADDRESS = value;
            break;
        case 0x2004:
            spr_ram[SPR_RAM_ADDRESS] = value;
            SPR_RAM_ADDRESS++;
            break;
        case 0x2005:  // based on http://wiki.nesdev.com/w/index.php/PPU_scrolling
            if (!W) { // first write
                T = (T & 0b1111111111100000) | (value >> 3);
                X = (value & 0b00000111);
                W = true;
            } else { // second write
                T = (T & 0b1000111111111111) | (((word)value & 0b00000111) << 12);
                T = (T & 0b1111110000011111) | (((word)value & 0b11111000) << 2);
                W = false;
            }
            break;
        case 0x2006: // based on http://wiki.nesdev.com/w/index.php/PPU_scrolling
            if (!W) { // first write
                T = (T & 0b1000000011111111) | ((((word)value) & 0b00111111) << 8);
                W = true;
            } else { // second write
                T = (T & 0b1111111100000000) | (word)value;
                V = T;
                W = false;
            }
            break;
        case 0x2007: // implement writing to vram
            //printf("V is %x before writing\n", V);
            ppu_mem_write(V, value);
            V += ADDRESS_INCREMENT; // every write to $2007 there is an increment of 1 or 31
            //printf("V is %x after writing\n", V);
            break;
        default:
            fprintf(stderr, "ERROR: Unrecognized PPU register write %X", address);
            break;
    }
}

byte read_ppu_register(word address) {
    static byte buffered = 0;
    static byte value = 0;
    byte temporary = buffered;
    switch(address) {
        case 0x2002:
            W = false;
            byte current = PPU_STATUS;
            // clear vblank on $2002 read
            PPU_STATUS &= 0b01111111;
            return current;
        case 0x2004:
            return spr_ram[SPR_RAM_ADDRESS];
        case 0x2007:
            value = ppu_mem_read(V);
            if (V % 0x4000 < 0x3F00) {
                buffered = value;
                value = temporary;
            } else {
                buffered = ppu_mem_read(V - 0x1000);
            }
            V += ADDRESS_INCREMENT; // every read to $2007 there is an increment of 1 or 32
            return value;
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
//        if (address >= 0x2800) {
//            printf("tried high read at %x\n", address);
//        }
        address = (address - 0x2000) % 0x1000; // 3000-3EFF is a mirror.
        if (rom->verticalMirroring) {
            address = address % 0x0800;
        } else { // horizontal mirroring
            if (address >= 0x400 && address < 0xC00) {
                address = address - 0x400;
            } else if (address >= 0xC00) {
                address = address - 0x800;
            }
        }
        return nametables[address];
    } else if (address < 0x4000) { // palette memory
        address = (address - 0x3F00) % 0x20;
        // $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C
        if ((address > 0x0F) && ((address % 0x04) == 0)) {
            address = address - 0x10;
        }
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
//        if (address >= 0x2000) {
//            printf("Writing %x at %x\n", value, address);
//        }
        address = (address - 0x2000) % 0x1000; // 3000-3EFF is a mirror.;
        if (rom->verticalMirroring) {
            address = address % 0x0800;
        } else { // horizontal mirroring
            if (address >= 0x400 && address < 0xC00) {
                address = address - 0x400;
            } else if (address >= 0xC00) {
                address = address - 0x800;
            }
        }
        
        nametables[address] = value;
    } else if (address < 0x4000) { // palette memory
        address = (address - 0x3F00) % 0x20;
        // $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C
        if ((address > 0x0F) && ((address % 0x04) == 0)) {
            address = address - 0x10;
        }
        palette[address] = value;
    } else {
        fprintf(stderr, "ERROR: Unrecognized PPU address write %X", address);
    }
}

// started from write to $4014
// fill spr_ram starting from cpuMemLocation
void dma_transfer(byte *cpuMemLocation) {
    memcpy(spr_ram, cpuMemLocation, SPR_RAM_SIZE);
}
