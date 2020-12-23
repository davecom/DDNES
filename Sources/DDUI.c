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

SDL_Window *nametable_window;
SDL_Renderer *nametable_renderer;
SDL_Texture *nametable_texture;

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
mtx_t nametable_debug_mutex;
mtx_t audio_mutex;
uint32_t *nametable_debug_pixels = NULL;
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

void start_stop_nametable_debug() {
    if (nametable_debug) { // start
        nametable_debug_pixels = malloc(sizeof(uint32_t) * NES_WIDTH * 2 * NES_HEIGHT * 2);
        mtx_init(&nametable_debug_mutex, mtx_plain);
        
        nametable_window = SDL_CreateWindow("Nametables", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, NES_WIDTH * 2, NES_HEIGHT * 2, SDL_WINDOW_ALLOW_HIGHDPI);
        if (nametable_window == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s", SDL_GetError());
            return;
        }
        nametable_renderer = SDL_CreateRenderer(nametable_window, -1, SDL_RENDERER_ACCELERATED);
        if (nametable_renderer == NULL) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create renderer: %s", SDL_GetError());
            return;
        }
        nametable_texture = SDL_CreateTexture(nametable_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, NES_WIDTH * 2, NES_HEIGHT * 2);
        SDL_SetRenderTarget(nametable_renderer, nametable_texture);
        SDL_SetRenderDrawColor(nametable_renderer, 0x00, 255, 0x00, 0x00);
        SDL_RenderClear(nametable_renderer);
    } else { // stop
        SDL_DestroyRenderer(nametable_renderer);
        SDL_DestroyWindow(nametable_window);
        SDL_DestroyTexture(nametable_texture);
        free(nametable_debug_pixels);
    }
}

int audio_buffer_length = 12800;
float audio_buffer[12800];
int audio_buffer_place = 0;
int last_audio_taken_from = 0;

void addAudioToBuffer(float a) {
    static int audio_ticks = 0;
    //mtx_lock(&audio_mutex);
    audio_buffer[audio_buffer_place] = a;
    audio_buffer_place++;
    if (audio_buffer_place >= audio_buffer_length) {
        audio_buffer_place = 0;
    }
    int skip = 82;
    if (audio_ticks % skip == 0 && audio_buffer_place >= skip) {
        float sample = 0;
        for (int i = 0; i < skip; i++) {
            sample += audio_buffer[audio_buffer_place - skip + i];
        }
        sample = sample / skip;
        SDL_QueueAudio(1, &sample, 4);
    }
    //SDL_QueueAudio(1, audio_buffer + audio_buffer_place, 4);
    audio_ticks++;
    //mtx_unlock(&audio_mutex);
    //if (audio_buffer_place - last_audio_taken_from > 77) {
    //    SDL_QueueAudio(1, audio_buffer + audio_buffer_place, 77);
    //    last_audio_taken_from = audio_buffer_place;
    //    //printf("queued at %d", audio_buffer_place);

    //}
    //if (last_audio_taken_from > audio_buffer_place) {
    //    last_audio_taken_from = audio_buffer_place;
    //}
    
}



#ifndef max
    #define max(a,b) ((a) > (b) ? (a) : (b))
#endif

void audio_call_back(void* userdata,
	Uint8* stream,
	int    len) {
	//mtx_lock(&audio_mutex);
	//if (audio_buffer_place == audio_buffer_length) {
	memset(stream, 0, len);
	float* fstream = (float*)stream;
	//printf("requested %d bytes\n", len);
	for (int i = 0; i < (len / 4); i++) {
		fstream[i] = audio_buffer[last_audio_taken_from];
        last_audio_taken_from++;
        if (last_audio_taken_from >= audio_buffer_length) {
            last_audio_taken_from = 0;
        }
	}
    //}
    //mtx_unlock(&audio_mutex);
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
                            //printf("start pressed");
                            joypad1.start = true;
                            //printf("%x", joypad1.start);
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
                        case SDLK_d:
                            debug = !debug;
                            break;
                        case SDLK_n:
                            nametable_debug = !nametable_debug;
                            start_stop_nametable_debug();
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
        // used to have this in but took it out and it seems to not
        // have any negative effect
        //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // black
        //SDL_RenderClear(renderer);
        
        
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
        
        if (nametable_debug) {
            SDL_SetRenderDrawColor(nametable_renderer, 0, 0, 0, 255); // black
            SDL_RenderClear(nametable_renderer);
            mtx_lock(&nametable_debug_mutex);
            //SDL_SetRenderTarget(renderer, texture);
            SDL_UpdateTexture(nametable_texture, NULL, nametable_debug_pixels, NES_WIDTH * 4 * 2);
            mtx_unlock(&nametable_debug_mutex);
            SDL_SetRenderTarget(nametable_renderer, NULL);
            SDL_RenderCopy(nametable_renderer, nametable_texture, NULL, NULL); // blit texture
            SDL_RenderPresent(nametable_renderer);
        }
        
//        mtx_lock(&audio_mutex);
//        //if (audio_buffer_place == audio_buffer_length) {
//
//            if (SDL_QueueAudio(1, audio_buffer, audio_buffer_place * 4) != 0) {
//                SDL_Log("Error queueing audio: %s", SDL_GetError());
//            }
//            audio_buffer_place = 0;
//        //}
//        mtx_unlock(&audio_mutex);
        
        //SDL_Delay(16);
        //printf("end drawing loop");
    }
    ui_cleanup();
}



void display_main_window(const char *title) {
    mtx_init(&pixel_list_mutex, mtx_plain);
    mtx_init(&frame_ready_mutex, mtx_plain);
    cnd_init(&frame_ready_condition);
    mtx_init(&audio_mutex, mtx_plain);
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return;
    }
    // startup audio
    SDL_AudioSpec want, have;

    SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
    want.freq = 22050;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 1024;
    want.callback = NULL; // muse use queueaudio

    if (SDL_OpenAudio(&want, &have) != 0) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
    } else {
        if (have.format != want.format) {
            SDL_Log("We didn't get Float32 audio format.");
        }
        SDL_PauseAudio(0); /* start audio playing. */
        //SDL_Delay(5000); /* let the audio callback play some sound for 5 seconds. */
        //SDL_CloseAudio();
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

void draw_nametables_pixel(int x, int y, byte palette_entry) {
    if (nametable_debug) {
        mtx_lock(&nametable_debug_mutex);
        nametable_debug_pixels[(x + y * NES_WIDTH * 2)] = nes_palette[palette_entry];
        mtx_unlock(&nametable_debug_mutex);
    }
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
