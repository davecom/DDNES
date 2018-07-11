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

typedef struct pixel_list {
    int x;
    int y;
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
    struct pixel_list *next;
} pixel_list;

pixel_list * head_pixel = NULL;
mtx_t pixel_list_mutex;

static void append_pixel(pixel_list *list) {
    mtx_lock(&pixel_list_mutex);
    if (head_pixel == NULL) {
        head_pixel = list;
    } else {
        pixel_list *current = head_pixel;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = list;
    }
    mtx_unlock(&pixel_list_mutex);
}

// caller is responsible for freeing the returned
// pixel_list
static pixel_list *pop_pixel() {
    mtx_lock(&pixel_list_mutex);
    if (head_pixel == NULL) {
        mtx_unlock(&pixel_list_mutex);
        return NULL;
    }
    pixel_list *temp = head_pixel;
    head_pixel = head_pixel->next;
    mtx_unlock(&pixel_list_mutex);
    return temp;
}

void event_loop() {
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
        // is there a pixel to draw?
        pixel_list *pixel = pop_pixel();
        if (pixel != NULL) {
            SDL_SetRenderDrawColor(renderer, pixel->r, pixel->g, pixel->b, pixel->a);
            SDL_RenderDrawPoint(renderer, pixel->x, pixel->y);
            SDL_RenderPresent(renderer);
            free(pixel); // responsible for freeing
        }
    }
    ui_cleanup();
}

void display_main_window(const char *title) {
    mtx_init(&pixel_list_mutex, mtx_plain);
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
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}

void draw_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) {
    pixel_list *pixel = malloc(sizeof(pixel_list));
    pixel->x = x;
    pixel->y = y;
    pixel->r = r;
    pixel->g = g;
    pixel->b = b;
    pixel->a = 255;
    pixel->next = NULL;
    append_pixel(pixel);
}

void ui_cleanup() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
