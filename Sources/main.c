//
//  main.c
//  DDNES
//
//  Created by David Kopec on 1/26/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include <stdio.h>
#include "DDROM.h"
#include "DDTest.h"

#define TEST

int main(int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World!\n");

    #ifdef TEST
    test();
    #endif
    
    return 0;
}
