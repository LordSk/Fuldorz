#ifndef UNICODE
    #define UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "imgui/gl3w.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

#define LOG(fmt, ...) (printf(fmt"\n", ##__VA_ARGS__))

struct AppWindow {

SDL_Window* window;
SDL_GLContext glContext;
bool running = true;

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

void doUI()
{
    ImGui::ShowDemoWindow();
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
    }
}

};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        LOG("SDL Error: %s", SDL_GetError());
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
