#pragma once

typedef struct SDL_Window {
    int w, h;
} SDL_Window;

SDL_Window* SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags) {
    SDL_Window* win = (SDL_Window*)malloc(sizeof(SDL_Window));
    win->w = w;
    win->h = h;
    return win;
}

void SDL_DestroyWindow(SDL_Window* window){
    free(window);
}

typedef struct SDL_Renderer {
    SDL_Window* window;
} SDL_Renderer;