//
//  main.c
//  DDNES
//
//  Created by David Kopec on 1/26/17.
//  Copyright © 2017 David Kopec. All rights reserved.
//

#include <stdio.h>

int main(int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World!\n");
    uint8_t x = 129;
    int y = ((char)x + (char)x + (char)x);
    printf("%d", y);
    return 0;
}
