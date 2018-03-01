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
    inline void resize(i32 w, i32 h);
    inline void moveToGpu();
};

struct AtlasInfo
{
    i32 columns;
    i32 rows;
};

struct IconAtlas
{
    Bitmap bmShell32;
    Bitmap bmImageres;
    Bitmap bmSysImgList;
    AtlasInfo atlasInfoShell32;
    AtlasInfo atlasInfoImageres;
    AtlasInfo aiSysImgList;
    i32 sysImgListCount = 0;

    struct SDL_Window* window;
    void* hdc;

    bool loadSystemIcons(struct SDL_Window *window_);
    bool updateSystemImageList();
    bool _loadDllIcons(const wchar_t* path, Bitmap *bmOut, AtlasInfo* atlasInfoOut);
    bool _loadSystemImageList(i32 from);
};
