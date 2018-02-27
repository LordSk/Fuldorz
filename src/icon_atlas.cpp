#include "icon_atlas.h"
#include <windows.h>
#include "imgui/gl3w.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

inline void Bitmap::alloc(i32 w, i32 h)
{
    assert(data == nullptr);
    data = (u32*)malloc(w * h * sizeof(u32));
    width = w;
    height = h;
}

inline void Bitmap::dealloc()
{
    assert(data);
    free(data);
}

bool IconAtlas::loadSystemIcons(SDL_Window* window_)
{
    window = window_;
    SDL_SysWMinfo sysInfo;
    SDL_VERSION(&sysInfo.version);
    SDL_GetWindowWMInfo(window, &sysInfo);
    HDC deviceContext = sysInfo.info.win.hdc;
    hdc = (void*)deviceContext;

    const wchar_t* pathShell32 = L"C:\\Windows\\system32\\shell32.dll";
    const wchar_t* pathImageRes = L"C:\\Windows\\system32\\imageres.dll";

    if(!_loadDllIcons(pathShell32, &bmShell32)) {
        return false;
    }

    if(!_loadDllIcons(pathImageRes, &bmImageRes)) {
        return false;
    }
    return true;
}

bool IconAtlas::_loadDllIcons(const wchar_t *path, Bitmap* bmOut)
{
    i32 iconCount = ExtractIconExW(path, -1, NULL, NULL, 0);
    if(iconCount <= 0) {
        LOG("ERROR> loadSystemIcons() failed to count icons", iconCount);
        return false;
    }

    HICON icons[1024];
    assert(iconCount <= 1024);
    i32 result = ExtractIconExW(path, 0, NULL, icons, iconCount);
    if(result != iconCount) {
        LOG("ERROR> loadSystemIcons() failed to extract %d icons (%d)", iconCount, result);
        return false;
    }

#if 0
    const i32 iconWidth = GetSystemMetrics(SM_CXSMICON);
    const i32 iconHeight = GetSystemMetrics(SM_CYSMICON);
#endif
    const i32 iconWidth = 16;
    const i32 iconHeight = 16;
    const i32 columnCount = 16;
    const i32 rowCount = iconCount/columnCount + 1;
    const i32 bitmapWidth = iconWidth * columnCount;
    const i32 bitmapheight = iconHeight * rowCount;

    bmOut->alloc(bitmapWidth, bitmapheight);

    u32 iconBmData[256*256];
    assert(iconWidth <= 256);
    assert(iconHeight <= 256);

    HDC deviceContext = (HDC)hdc;

    for(i32 i = 0; i < iconCount; ++i) {
        ICONINFO info = {};
        GetIconInfo(icons[i], &info);

        BITMAPINFO bmInfo = {0};
        bmInfo.bmiHeader.biSize = sizeof(bmInfo.bmiHeader);

        if(GetDIBits(deviceContext, info.hbmColor, 0, 0,
                     NULL, &bmInfo, DIB_RGB_COLORS) == 0) {
            LOG("ERROR> loadSystemIcons() failed to extract bitmap info (%d)", i, GetLastError());
            return false;
        }

        bmInfo.bmiHeader.biBitCount = 32;
        bmInfo.bmiHeader.biCompression = BI_RGB;
        bmInfo.bmiHeader.biHeight = -abs(bmInfo.bmiHeader.biHeight);

        if(GetDIBits(deviceContext, info.hbmColor, 0, bmInfo.bmiHeader.biHeight,
                     iconBmData, &bmInfo, DIB_RGB_COLORS) == 0) {
            LOG("ERROR> loadSystemIcons() failed to extract bitmap data (%d)", i, GetLastError());
            return false;
        }

        const i32 atlasX = i % columnCount;
        const i32 atlasY = i / columnCount;
        const i32 atlasOffset = (atlasY * columnCount * iconWidth * iconHeight) +
                atlasX * iconWidth;
        for(i32 l = 0; l < iconHeight; ++l) {
            memmove(&bmOut->data[atlasOffset + (columnCount * iconWidth * l)],
                    &iconBmData[l * iconWidth],
                    sizeof(u32) * iconWidth);
        }
    }

    for(i32 i = 0; i < iconCount; ++i) {
        DestroyIcon(icons[i]);
    }

    glGenTextures(1, &bmOut->gpuTexId);
    glBindTexture(GL_TEXTURE_2D, bmOut->gpuTexId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmapWidth, bitmapheight, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, bmOut->data);

    return true;
}
