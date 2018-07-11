//
//  main.c
//  DDNES
//
//  Created by David Kopec on 1/26/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include <stdio.h>
// temporary until clang supports <threads.h>
// from the c11 standard
#include "c11threads.h"
#include "DDROM.h"
#include "DD6502.h"
#include "DDTest.h"
#include "DDTypes.h"
#include "DDUI.h"


extern uint64_t cpu_ticks;

int emulate(void *data) {
    cpu_reset();
    while(1) { // run 8992 instructions
        uint64_t last_ticks = cpu_ticks;
        cpu_cycle();
        uint64_t difference = cpu_ticks - last_ticks;
        for (int j = 0; j < (difference * 3); j++) { // 3 ppu ticks for every 1 cpu tick
            ppu_step();
        }
    }
    unloadROM();
    return 0;
}

int main(int argc, const char * argv[]) {
    // printf("Hello, World! %.4X\n", 84 << 0 | 85 << 8);

    #ifdef TEST // if we're testing run the tests
    test();
    #else // otherwise try to load the game specified on the command line
    if (argc != 2) {
        printf("Expected exactly 1 command-line argument—the name of the ROM file to execute.");
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
