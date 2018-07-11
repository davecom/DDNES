//
//  DDTest.c
//  DDNES
//
//  Created by David Kopec on 2/10/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//
#ifdef TEST // if we're testing run the tests
#include "DDTest.h"
#include "DD6502.h"
#include "DDROM.h"

bool CPUTest1() {
    if (!loadROM("TestROMs/official_only.nes")) { // load the rom
        return false;
    }
    printf("Mapper %d\n", rom->mapper);
    cpu_reset();
    for (int i = 0; i < 3000; i++) { // run 3000 instructions
        cpu_cycle();
    }
    unloadROM();
    return true;
}

bool CPUTest2() {
    if (!loadROM("TestROMs/nestest.nes")) { // load the rom
        return false;
    }
    printf("Mapper %d\n", rom->mapper);
    cpu_reset();
    PC_Move(0xC000); // for automated testing
    for (int i = 0; i < 8992; i++) { // run 8992 instructions
        cpu_cycle();
    }
    unloadROM();
    return true;
}

void test() {
    int attempted = 0;
    int successful = 0;
    
    // CPU TEST 1
    attempted++;
    if (!CPUTest1()) {
        printf("CPUTest 1 Failed!\n");
    } else {
        successful++;
    }
    
    // CPU TEST 2
    attempted++;
    if (!CPUTest2()) {
        printf("CPUTest 2 Failed!\n");
    } else {
        successful++;
    }
    
    // ... other tests called here with printouts if failed
    
    printf("Passed %d of %d attempted tests.\n", successful, attempted);
}
#endif
