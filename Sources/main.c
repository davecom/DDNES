//
//  main.c
//  DDNES
//
//  Created by David Kopec on 1/26/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>
// temporary until clang supports <threads.h>
// from the c11 standard
#include "tinycthread.h"
#include "DDROM.h"
#include "DD6502.h"
#include "DDTest.h"
#include "DDTypes.h"
#include "DDUI.h"
#include "DDPPU.h"

#define NANOSECONDS_PER_CPU_CYCLE 559 // 559 for every 1 CPU cycle
#define NANOSECONDS_PER_FRAME 16666666
#define CPU_CYLES_PER_FRAME 29830
#define MILLISECONDS_PER_FRAME 16


extern uint64_t cpu_ticks;

int emulate(void *data) {
    cpu_reset();
    //ppu_reset();
    uint64_t ticks_since_last_delay = 0;
    uint32_t time = SDL_GetTicks();
    while(1) { 
        uint64_t last_ticks = cpu_ticks;
        cpu_cycle();
        uint64_t difference = cpu_ticks - last_ticks;
        for (int j = 0; j < (difference * 3); j++) { // 3 ppu ticks for every 1 cpu tick
            ppu_step();
        }
        
        // 1 apu tick for every cpu tick
        for (int j = 0; j < difference; j++) {
            apu_tick();
        }
        
        // pause if we are going too fast
        ticks_since_last_delay += difference;
        if (ticks_since_last_delay > CPU_CYLES_PER_FRAME) {
            uint32_t cur_time = SDL_GetTicks();
            uint32_t time_difference = (cur_time - time);
            if (time_difference < MILLISECONDS_PER_FRAME) {
                //printf("Too little at %d", time_difference);
                struct timespec how_long;
                how_long.tv_sec = 0;
                how_long.tv_nsec = (MILLISECONDS_PER_FRAME - time_difference - 2) * 1000000;
                // SDL_Delay is not thread safe
                //SDL_Delay(MILLISECONDS_PER_FRAME - time_difference);
                thrd_sleep(&how_long, NULL);
            }
            time = SDL_GetTicks();
            ticks_since_last_delay = 0;
        }
    }
    //unloadROM();
    return 0;
}

int main(int argc, char * argv[]) {
    // printf("Hello, World! %.4X\n", 84 << 0 | 85 << 8);

    #ifdef TEST // if we're testing run the tests
    test();
    #else // otherwise try to load the game specified on the command line
    if (argc != 2) {
        printf("Expected exactly 1 command-line argument—the path of the ROM file to execute.");
        return 1;
    }
    if (!loadROM(argv[1])) { // load the rom
        return 1;
    }
    printf("Mapper %d\n", rom->mapper);
    // create ui
    display_main_window(argv[1]);
    // start separate thread for emulator
    thrd_t emuthr;
    
    thrd_create(&emuthr, emulate, NULL);
    // start ui event loop
    event_loop();
    #endif
    return 0;
}
