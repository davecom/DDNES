//
//  DDTest.c
//  DDNES
//
//  Created by David Kopec on 2/10/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include "DDTest.h"
#include "DD6502.h"
#include "DDROM.h"

bool CPUTest() {
    if (!loadROM("TestROMs/official_only.nes")) { // load the rom
        return false;
    }
    return true;
}

void test() {
    int attempted = 0;
    int successful = 0;
    
    // CPU TEST
    attempted++;
    if (!CPUTest()) {
        printf("CPUTest Failed!\n");
    } else {
        successful++;
    }
    
    // ... other tests called here with printouts if failed
    
    printf("Passed %d of %d attempted tests.\n", successful, attempted);
}
