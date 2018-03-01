#ifndef UNICODE
    #define UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "imgui/gl3w.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include "base.h"
#include "utils.h"
#include "icon_atlas.h"

// TODO: use 0xed window setup

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900
#define FS_MAX_ENTRIES 1024

struct AppWindow {

SDL_Window* window;
SDL_GLContext glContext;
bool running = true;
FileSystemEntry fsList[FS_MAX_ENTRIES];
i32 fsListCount = 0;
Path currentDir;

IconAtlas iconAtlas;

bool init()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    window = SDL_CreateWindow("Fuldorz",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT,
                              SDL_WINDOW_OPENGL);

    if(!window) {
        LOG("ERROR: can't create SDL2 window (%s)",  SDL_GetError());
        return false;
    }

    glContext = SDL_GL_CreateContext(window);
    if(!glContext) {
        LOG("ERROR: can't create OpenGL 3.3 context (%s)",  SDL_GetError());
        return false;
    }

    SDL_GL_SetSwapInterval(0);

    if(gl3w_init()) {
        LOG("ERROR: can't init gl3w");
        return false;
    }

    if(!gl3w_is_supported(3, 3)) {
        LOG("ERROR: OpenGL 3.3 isn't available on this system");
        return false;
    }

    ImGui_ImplSdlGL3_Init(window, "fuld0rz_imgui.ini");
    ImGui::StyleColorsLight();

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahoma.ttf", 15);

    glClearColor(1, 1, 1, 1);

    if(!iconAtlas.loadSystemIcons(window)) {
        return false;
    }

    //currentDir.set(L"C:");
    currentDir.set(L"C:\\Prog\\Projets\\Fuldorz");
    updateFileList();
    return true;
}

void cleanup()
{
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
}

void run()
{
    while(running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            processEvent(&event);
        }
        ImGui_ImplSdlGL3_NewFrame(window);

        doUI();

        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        SDL_GL_SwapWindow(window);
    }

    cleanup();
}

void ImGui_Path(Path& path)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 2));

    ImGui::BeginGroup();

    char buff[256];

    for(i32 f = 0; f < path.folderCount; ++f) {
        const i32 folderLen = path.folder[f].nameLen;
        toUtf8(path.folder[f].name, buff, sizeof(buff), folderLen);
        buff[folderLen] = 0;

        if(ImGui::Button(buff)) {
            path.goUp(path.folderCount - f - 1);
            updateFileList();
        }
        if(f+1 < path.folderCount) {
            ImGui::SameLine();
        }
    }

    ImGui::EndGroup();

    ImGui::PopStyleVar(1);
}

void doUI()
{
    //ImGui::ShowDemoWindow();

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH/2,WINDOW_HEIGHT));
    ImGui::Begin("##left_window", nullptr,
                 ImGuiWindowFlags_NoMove|
                 ImGuiWindowFlags_NoTitleBar|
                 ImGuiWindowFlags_NoResize|
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui_Path(currentDir);

    ImGui::BeginChild("##fs_item_list", ImVec2(-1, -1));


    char name[256];
    for(i32 i = 0; i < fsListCount; ++i) {
        if(fsList[i].isSpecial()) continue;

        i32 iconId = fsList[i].icon;
        ImTextureID iconAtlasTex = 0;
        i32 c, r;
        if(iconId < 0) {
            iconAtlasTex = (ImTextureID)(i64)iconAtlas.bmImageres.gpuTexId;
            c = iconAtlas.atlasInfoImageres.columns;
            r = iconAtlas.atlasInfoImageres.rows;
            iconId = -iconId;

        }
        else {
            assert(iconId < iconAtlas.sysImgListCount);
            /*iconAtlasTex = (ImTextureID)(i64)iconAtlas.bmImageres.gpuTexId;
            c = iconAtlas.atlasInfoImageres.columns;
            r = iconAtlas.atlasInfoImageres.rows;*/
#if 1
            iconAtlasTex = (ImTextureID)(i64)iconAtlas.bmSysImgList.gpuTexId;
            c = iconAtlas.aiSysImgList.columns;
            r = iconAtlas.aiSysImgList.rows;
#endif
        }

        ImVec2 uv0((iconId % c) / (f32)c, (iconId / c) / (f32)r);
        ImVec2 uv1(((iconId % c) + 1) / (f32)c, ((iconId / c) + 1) / (f32)r);

        ImGui::Image(iconAtlasTex, ImVec2(16, 16), uv0, uv1);
        ImGui::SameLine();

        /*
        ImGui::Text("%04d", fsList[i].icon);
        ImGui::SameLine();
        */


        fsList[i].name.toUtf8(name, 256);
        if(fsList[i].isDir()) {
            if(ImGui::Button(name)) {
                currentDir.goDown(fsList[i].name.data);
                updateFileList();
            }
        }
        else {
            ImGui::TextUnformatted(name);
        }
    }

    ImGui::EndChild();

    ImGui::End();

    ImGui::Begin("Debug");

    /*if(ImGui::CollapsingHeader("shell32")) {
        ImGui::Image((ImTextureID)(i64)iconAtlas.bmShell32.gpuTexId,
                     ImVec2(iconAtlas.bmShell32.width, iconAtlas.bmShell32.height));
    }
    if(ImGui::CollapsingHeader("imageres")) {
        ImGui::Image((ImTextureID)(i64)iconAtlas.bmImageres.gpuTexId,
                     ImVec2(iconAtlas.bmImageres.width, iconAtlas.bmImageres.height));
    }*/
    if(ImGui::CollapsingHeader("system image list")) {
        ImGui::Image((ImTextureID)(i64)iconAtlas.bmSysImgList.gpuTexId,
                     ImVec2(iconAtlas.bmSysImgList.width, iconAtlas.bmSysImgList.height));
    }

    ImGui::End();
}

void processEvent(SDL_Event* event)
{
    if(event->type == SDL_QUIT) {
        running = false;
        return;
    }

    if(event->type == SDL_KEYDOWN) {
        if(event->key.keysym.sym == SDLK_ESCAPE) {
            running = false;
            return;
        }
        if(event->key.keysym.sym == SDLK_BACKSPACE) {
            currentDir.goUp();
            updateFileList();
            return;
        }
    }
}

void updateFileList()
{
    listFsEntries(currentDir.getStr(), fsList, &fsListCount);

    bool foundOne = false;
    for(i32 i = 0; i < fsListCount && !foundOne; ++i) {
        if(fsList[i].icon >= iconAtlas.sysImgListCount) {
            foundOne = true;
        }
    }

    if(foundOne) {
        iconAtlas.updateSystemImageList();
    }
}

};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    setbuf(stdout, NULL);
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        LOG("SDL Error: %s", SDL_GetError());
        return 1;
    }

    // TODO: change this to multi threaded (CoInitializeEx)
    if(CoInitialize(NULL) != S_OK) {
        return 1;
    }

    AppWindow app;
    if(!app.init()) {
        return 1;
    }
    app.run();

    SDL_Quit();
    return 0;
}
