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
SDL_Texture *texture;

//typedef struct pixel_list {
//    int x;
//    int y;
//    Uint8 r;
//    Uint8 g;
//    Uint8 b;
//    Uint8 a;
//    struct pixel_list *next;
//} pixel_list;
//
//pixel_list * head_pixel = NULL;
mtx_t pixel_list_mutex;
uint32_t pixels[(NES_HEIGHT * NES_WIDTH)];


//static void append_pixel(pixel_list *list) {
//    mtx_lock(&pixel_list_mutex);
//    if (head_pixel == NULL) {
//        head_pixel = list;
//    } else {
//        pixel_list *current = head_pixel;
//        while (current->next != NULL) {
//            current = current->next;
//        }
//        current->next = list;
//    }
//    mtx_unlock(&pixel_list_mutex);
//}
//
//// caller is responsible for freeing the returned
//// pixel_list
//static pixel_list *pop_pixel() {
//    if (head_pixel == NULL) {
//        return NULL;
//    }
//    mtx_lock(&pixel_list_mutex);
//    pixel_list *temp = head_pixel;
//    head_pixel = head_pixel->next;
//    mtx_unlock(&pixel_list_mutex);
//    return temp;
//}

void event_loop() {
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
        // clear renderer
        
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // black
        SDL_RenderClear(renderer);
        // is there a pixel to draw?
//        pixel_list *pixel = pop_pixel();
//        while (pixel != NULL) {
//            SDL_SetRenderTarget(renderer, texture);
//            SDL_SetRenderDrawColor(renderer, pixel->r, pixel->g, pixel->b, pixel->a);
//            SDL_RenderDrawPoint(renderer, pixel->x, pixel->y);
//            free(pixel); // responsible for freeing
//            pixel = pop_pixel();
//        }
        
        mtx_lock(&pixel_list_mutex);
        //SDL_SetRenderTarget(renderer, texture);
        SDL_UpdateTexture(texture, NULL, pixels, NES_WIDTH * 4);
        mtx_unlock(&pixel_list_mutex);
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL); // blit texture
        SDL_RenderPresent(renderer);
        
        SDL_Delay(10);
        //printf("end drawing loop");
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
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, NES_WIDTH, NES_HEIGHT);
    SDL_SetRenderTarget(renderer, texture);
    SDL_SetRenderDrawColor(renderer, 0x00, 255, 0x00, 0x00);
    SDL_RenderClear(renderer);
}

void draw_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) {
//    pixel_list *pixel = malloc(sizeof(pixel_list));
//    pixel->x = x;
//    pixel->y = y;
//    pixel->r = r;
//    pixel->g = g;
//    pixel->b = b;
//    pixel->a = 255;
//    pixel->next = NULL;
    mtx_lock(&pixel_list_mutex);
    uint32_t temp = 0;
    temp |= (r << 24);
    temp |= (g << 16);
    temp |= (b << 8);
    temp |= 255;
    pixels[(x + y * NES_WIDTH)] = temp;
    mtx_unlock(&pixel_list_mutex);
//    append_pixel(pixel);
}

void ui_cleanup() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(texture);
    SDL_Quit();
}
