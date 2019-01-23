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
mtx_t frame_ready_mutex;
cnd_t frame_ready_condition;
uint32_t pixels[(NES_HEIGHT * NES_WIDTH)]; // pixel buffer
uint32_t nes_palette[64] = {0x7C7C7CFF, 0x0000FCFF, 0x0000BCFF, 0x4428BCFF, 0x940084FF, 0xA80020FF, 0xA81000FF, 0x881400FF, 0x503000FF, 0x007800FF, 0x006800FF, 0x005800FF, 0x004058FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xBCBCBCFF, 0x0078F8FF, 0x0058F8FF, 0x6844FCFF, 0xD800CCFF, 0xE40058FF, 0xF83800FF, 0xE45C10FF, 0xAC7C00FF, 0x00B800FF, 0x00A800FF, 0x00A844FF, 0x008888FF, 0x000000FF, 0x000000FF, 0x000000FF, 0xF8F8F8FF, 0x3CBCFCFF, 0x6888FCFF, 0x9878F8FF, 0xF878F8FF, 0xF85898FF, 0xF87858FF, 0xFCA044FF, 0xF8B800FF, 0xB8F818FF, 0x58D854FF, 0x58F898FF, 0x00E8D8FF, 0x787878FF, 0x000000FF, 0x000000FF, 0xFCFCFCFF, 0xA4E4FCFF, 0xB8B8F8FF, 0xD8B8F8FF, 0xF8B8F8FF, 0xF8A4C0FF, 0xF0D0B0FF, 0xFCE0A8FF, 0xF8D878FF, 0xD8F878FF, 0xB8F8B8FF, 0xB8F8D8FF, 0x00FCFCFF, 0xF8D8F8FF, 0x000000FF, 0x000000FF};

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

void frame_ready() {
    cnd_signal(&frame_ready_condition);
}

void event_loop() {
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    switch (e.key.keysym.sym) {
                        case SDLK_x:
                            joypad1.a = true;
                            break;
                        case SDLK_z:
                            joypad1.b = true;
                            break;
                        case SDLK_s:
                            printf("start pressed");
                            joypad1.start = true;
                            printf("%x", joypad1.start);
                            break;
                        case SDLK_a:
                            joypad1.select = true;
                            break;
                        case SDLK_UP:
                            joypad1.up = true;
                            break;
                        case SDLK_DOWN:
                            joypad1.down = true;
                            break;
                        case SDLK_LEFT:
                            joypad1.left = true;
                            break;
                        case SDLK_RIGHT:
                            joypad1.right = true;
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    switch (e.key.keysym.sym) {
                        case SDLK_x:
                            joypad1.a = false;
                            break;
                        case SDLK_z:
                            joypad1.b = false;
                            break;
                        case SDLK_s:
                            joypad1.start = false;
                            break;
                        case SDLK_a:
                            joypad1.select = false;
                            break;
                        case SDLK_UP:
                            joypad1.up = false;
                            break;
                        case SDLK_DOWN:
                            joypad1.down = false;
                            break;
                        case SDLK_LEFT:
                            joypad1.left = false;
                            break;
                        case SDLK_RIGHT:
                            joypad1.right = false;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }
        // clear renderer
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black
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
        //cnd_wait(&frame_ready_condition, &frame_ready_mutex);
        mtx_lock(&pixel_list_mutex);
        //SDL_SetRenderTarget(renderer, texture);
        SDL_UpdateTexture(texture, NULL, pixels, NES_WIDTH * 4);
        mtx_unlock(&pixel_list_mutex);
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL); // blit texture
        SDL_RenderPresent(renderer);
        
        //SDL_Delay(16);
        //printf("end drawing loop");
    }
    ui_cleanup();
}

void display_main_window(const char *title) {
    mtx_init(&pixel_list_mutex, mtx_plain);
    mtx_init(&frame_ready_mutex, mtx_plain);
    cnd_init(&frame_ready_condition);
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

void draw_pixel(int x, int y, byte palette_entry) {
    //printf("%d\n", palette_entry);
    mtx_lock(&pixel_list_mutex);
    pixels[(x + y * NES_WIDTH)] = nes_palette[palette_entry];
    mtx_unlock(&pixel_list_mutex);
}

//void draw_pixel(int x, int y, Uint8 r, Uint8 g, Uint8 b) {
////    pixel_list *pixel = malloc(sizeof(pixel_list));
////    pixel->x = x;
////    pixel->y = y;
////    pixel->r = r;
////    pixel->g = g;
////    pixel->b = b;
////    pixel->a = 255;
////    pixel->next = NULL;
//    mtx_lock(&pixel_list_mutex);
//    uint32_t temp = 0;
//    temp |= (r << 24);
//    temp |= (g << 16);
//    temp |= (b << 8);
//    temp |= 255;
//    pixels[(x + y * NES_WIDTH)] = temp;
//    mtx_unlock(&pixel_list_mutex);
////    append_pixel(pixel);
//}

void ui_cleanup() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(texture);
    SDL_Quit();
}
