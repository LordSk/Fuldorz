#pragma once
#include "base.h"

struct Bitmap
{
    u32* data = nullptr;
    i32 width;
    i32 height;
    u32 gpuTexId = 0;

    inline void alloc(i32 w, i32 h);
    inline void dealloc();
};

struct IconAtlas
{
    Bitmap bmShell32;
    Bitmap bmImageRes;
    struct SDL_Window* window;
    void* hdc;

    bool loadSystemIcons(struct SDL_Window *window_);
    bool _loadDllIcons(const wchar_t* path, Bitmap *bmOut);
};
