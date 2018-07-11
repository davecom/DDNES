//
//  DDUI.c
//  DDNES
//
//  Created by David Kopec on 7/10/18.
//  Copyright © 2018 David Kopec. All rights reserved.
//

#include "DDUI.h"

SDL_Window *window;
SDL_Renderer *renderer;

void event_loop() {
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
    }
    ui_cleanup();
}

void display_main_window(const char *title) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return;
    }
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, NES_WIDTH, NES_HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s", SDL_GetError());
        return;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create renderer: %s", SDL_GetError());
        return;
    }
    event_loop();
}

void draw_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) {
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    SDL_RenderPresent(renderer);
}

void ui_cleanup() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
