//
//  DDAPU.c
//  DDNES
//
//  Created by David Kopec on 11/10/19.
//  Copyright © 2019 David Kopec. All rights reserved.
//

#include "DDAPU.h"

// APU Register referenced from docs at http://wiki.nesdev.com/w/index.php/APU
// Based on Pseudocode from http://forums.nesdev.com/viewtopic.php?f=3&t=13767&hilit=audio+psuedo

byte length_table[32] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

bool duty_cycle[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1,}
};

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
    bool sweep_reload : 1;
    byte sweep_counter : 3;
    
    word timer : 11; // timer low and timer high together
    byte length : 5; // Length counter load (L)
    word timer_count : 11;
    byte length_count : 8;
    byte duty_count : 3;
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

void write40004004(struct pulse *p, byte value) {
    p->duty = (value >> 6) & 3;
    p->halt = (value >> 5) & 1;
    p->constant_volume = (value >> 4) & 1;
    p->volume = value & 0x0F;
}

void write40014005(struct pulse *p, byte value) {
    p->sweep_enabled = (value >> 7) & 1;
    p->sweep_period = (value >> 4) & 7;
    p->sweep_negate = (value >> 3) & 1;
    p->sweep_shift = value & 7;
    p->sweep_reload = true;
}

void write_apu_register(word address, byte value) {
    //printf("wrote %X to %X in APU", value, address);
    // switch on address; writing to appropriate places; break after each case
    switch (address) {
        case 0x4000:
            write40004004(&pulse1, value);
            break;
        case 0x4001:
            write40014005(&pulse1, value);
            break;
        case 0x4002:
            pulse1.timer = (pulse1.timer & (0x700)) | value;
            break;
        case 0x4003:
            pulse1.timer = (pulse1.timer & (0x0FF)) | (((word)value & 7) << 8);
            pulse1.timer_count = pulse1.timer;
            pulse1.duty_count = 0;
            pulse1.length = (value >> 3) & 0x1F;
            pulse1.length_count = length_table[pulse1.length];
            break;
        case 0x4004:
            write40004004(&pulse2, value);
            break;
        case 0x4005:
            write40014005(&pulse2, value);
            break;
        case 0x4006:
            pulse2.timer = (pulse2.timer & (0x700)) | value;
            break;
        case 0x4007:
            pulse2.timer = (pulse2.timer & (0x0FF)) | (((word)value & 7) << 8);
            pulse2.timer_count = pulse2.timer;
            pulse2.duty_count = 0;
            pulse2.length = (value >> 3) & 0x1F;
            pulse2.length_count = length_table[pulse2.length];
            break;
        case 0x4015:
            if ((value & 1) == 0) {
                pulse1.length = 0;
            }
            if ((value & 2) == 0) {
                pulse2.length = 0;
            }
            break;
        case 0x4017:
            frame_counter.mode = (value & 0x80) >> 7;
            frame_counter.irq_inhibit = (value & 0x40) >> 6;
            frame_counter.count = 0;
            break;
        default:
            break;
    }
}

float pulse_envelope(struct pulse *p) {
    if (duty_cycle[p->duty][p->duty_count] && p->timer >= 8 && p->timer <= 0x7FF && p->length_count > 0) {
        return p->volume;
    }
    return 0.0;
}

float generate_pulse() {
    
    float p1o = (float) pulse_envelope(&pulse1);
    float p2o = (float) pulse_envelope(&pulse2);
    float p2t = (p1o + p2o);
    //return p2t;
    //if (p2t <= 0.00001) { return 0.0; }
    //float pulse_out = 95.88 / ((float)(8128 / p2t) + 100);
	float pulse_out = 0.00752 * (p2t); // linear approximation * 10 is a hack
	return pulse_out;
}

void tick_pulse(struct pulse *p) {
    if (p->timer_count == 0) {
        p->timer_count = p->timer;
        p->duty_count++;
        if (p->duty_count > 7) {
            p->duty_count = 0;
        }
    } else {
        p->timer_count--;
    }
}

uint64_t apu_ticks;

extern int audio_buffer_length;
extern float audio_buffer[128000];
extern int audio_buffer_place;

void apu_tick() {
    
    if (apu_ticks % 2 == 0) { // every other cpu tick
        
        
        tick_pulse(&pulse1);
        tick_pulse(&pulse2);
        
        float po = generate_pulse();//generate_pulse();
        /*if (po > 0.05) {
            printf("generated po of %f", po);
        }*/
        /*audio_buffer[audio_buffer_place] = po * 10;
        audio_buffer_place++;*/
        addAudioToBuffer(po);
		/*if (audio_buffer_place >= 128000) {
			SDL_QueueAudio(1, audio_buffer, 512000);
			audio_buffer_place = 0;
		}*/
		
    }

    
    if (apu_ticks % 7457 == 0) { // new frame count
        
        frame_counter.count++;
        
        // envelope and triangle linear counter (quarter frame)
        if (frame_counter.count == 1 || frame_counter.count == 2 || frame_counter.count == 3 || frame_counter.count == 5) {
            if (pulse1.halt && !pulse1.constant_volume) {
                if (pulse1.volume == 0) {
                    pulse1.volume = 15;
                } else {
                    pulse1.volume--;
                }
                
            }
            if (pulse2.halt && !pulse2.constant_volume) {
                if (pulse2.volume == 0) {
                    pulse2.volume = 15;
                }
                else {
                    pulse2.volume--;
                }

            }
        }

        // half frame; sweep unit and length counter
        if (frame_counter.count == 2 || frame_counter.count == 5) {
            if (pulse1.sweep_reload) {
                pulse1.sweep_counter = pulse1.sweep_period;
                pulse1.sweep_reload = false;
            }
            else if (pulse1.sweep_counter > 0) {
                pulse1.sweep_counter--;
            }
            else {
                pulse1.sweep_counter = pulse1.sweep_period;
                if (pulse1.sweep_enabled) {
                    word sweep_change = pulse1.timer >> pulse1.sweep_shift;
                    if (pulse1.sweep_negate) {
                        pulse1.timer -= sweep_change + 1;
                    }
                    else {
                        pulse1.timer += sweep_change;
                    }
                }
            }

            if (pulse2.sweep_reload) {
                pulse2.sweep_counter = pulse2.sweep_period;
                pulse2.sweep_reload = false;
            }
            else if (pulse2.sweep_counter > 0) {
                pulse2.sweep_counter--;
            }
            else {
                pulse2.sweep_counter = pulse2.sweep_period;
                if (pulse2.sweep_enabled) {
                    word sweep_change = pulse2.timer >> pulse2.sweep_shift;
                    if (pulse2.sweep_negate) {
                        pulse2.timer -= sweep_change;
                    }
                    else {
                        pulse2.timer += sweep_change;
                    }
                }
            }
            
            if (!pulse1.halt) {
                if (pulse1.length_count > 0) {
                    pulse1.length_count--;
                }
            }
            if (!pulse2.halt) {
                if (pulse2.length_count > 0) {
                    pulse2.length_count--;
                }
            }
        }
        
        if (frame_counter.count == 4 && !frame_counter.mode) {
            frame_counter.count = 0;
        } else if (frame_counter.count == 5) {
            frame_counter.count = 0;
        }
    }
    apu_ticks++;
}

void apu_reset() {
    memset(&pulse1, 0, sizeof(pulse1));
    memset(&pulse2, 0, sizeof(pulse2));
    apu_ticks = 0;
}
