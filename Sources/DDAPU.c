//
//  DDAPU.c
//  DDNES
//
//  Created by David Kopec on 11/10/19.
//  Copyright © 2019 David Kopec. All rights reserved.
//

#include "DDAPU.h"

// APU Register referenced from docs at http://wiki.nesdev.com/w/index.php/APU

struct pulse {
    byte duty : 2; // Duty (D)
    bool halt : 1; // envelope loop / length counter halt (L)
    bool constant_volume : 1; // constant volume (C)
    byte volume : 4; // volume/envelope (V)
    // Sweep unit: enabled (E), period (P), negate (N), shift (S)
    bool sweep_enabled : 1;
    byte sweep_period : 3;
    bool sweep_negate : 1;
    byte sweep_shift : 3;
    
    word timer : 11; // timer low and timer high together
    byte length : 5; // Length counter load (L)
} pulse1, pulse2;

struct {
    bool halt : 1; // Length counter halt / linear counter control (C)
    byte linear_counter : 7; // linear counter load (R)
    word timer : 11; // timer low and timer high together
    byte length : 5; // Length counter load (L)
} triangle;

struct {
    bool halt : 1; // envelope loop / length counter halt (L)
    bool constant_volume : 1; // constant volume (C)
    byte volume : 4; // volume/envelope (V)
    bool loop_noise : 1; // Loop noise (L)
    byte noise_period : 4; // noise period (P)
    byte length : 5; // Length counter load (L)
} noise;

struct {
    bool irq_enable : 1;
    bool loop : 1;
    byte frequency : 4;
    byte load_counter : 7;
    byte sample_address : 8;
    byte sample_length : 8;
} dmc;

// frame counter stuff from nesdev
//mode 0:    mode 1:       function
//---------  -----------  -----------------------------
// - - - f    - - - - -    IRQ (if bit 6 is clear)
// - l - l    - l - - l    Length counter and sweep
// e e e e    e e e - e    Envelope and linear counter

struct {
    bool mode : 1; // Mode (M, 0 = 4-step, 1 = 5-step)
    bool irq_inhibit : 1; // IRQ inhibit flag (I)
    byte count : 3; // 240 hz; ranges from 0-5 depending on mode
} frame_counter;

byte read_apu_status() {
    byte status = (0 | (dmc.irq_enable << 7) | (frame_counter.irq_inhibit << 6) |  ((dmc.sample_length > 0) << 4) | ((noise.length > 0) << 3) | ((triangle.length > 0) << 2) | ((pulse2.length > 0) << 1) | (pulse1.length > 0));
    // "Reading this register clears the frame interrupt flag
    // (but not the DMC interrupt flag)."
    frame_counter.irq_inhibit = 0;
    return status;
}

void write_apu_register(word address, byte value) {
    // switch on address; writing to appropriate places; break after each case
    switch (address) {
        default:
            break;
    }
}
