//
//  DDUI.h
//  DDNES
//
//  Created by David Kopec on 7/10/18.
//  Copyright © 2018 David Kopec. All rights reserved.
//

#ifndef DDUI_h
#define DDUI_h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
// Apple OSX and iOS (Darwin)
#if defined(__APPLE__) && defined(__MACH__)
#include <SDL2/SDL.h>
#else 
#include <SDL.h>
#endif
#include "tinycthread.h"
#include "DDTypes.h"
#include "DDInput.h"
#include "DDPPU.h"

#define NES_WIDTH 256
#define NES_HEIGHT 240

void event_loop(void);
void frame_ready(void);
void display_main_window(const char *title);
void draw_nametables_pixel(int x, int y, byte palette_entry);
void draw_pixel(int x, int y, byte palette_entry); //Uint8 r, Uint8 g, Uint8 b);
void ui_cleanup(void);

#endif /* DDUI_h */
