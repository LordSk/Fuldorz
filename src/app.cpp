#ifndef UNICODE
    #define UNICODE
#endif
#include <windows.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "imgui/gl3w.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"

// TODO: use 0xed window setup

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

#define LOG(fmt, ...) (printf(#fmt"\n", ##__VA_ARGS__))
#define LOGU(fmt, ...) (wprintf(fmt L"\n", ##__VA_ARGS__))

typedef uint8_t u8;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;

template<u32 STR_SIZE>
struct StrU
{
    wchar_t data[STR_SIZE];
    i32 length;

    void set(const wchar_t* src) {
        length = wcslen(src);
        assert(length < STR_SIZE);
        memmove(data, src, length * sizeof(data[0]));
        data[length] = 0;
    }

    void setFmt(const wchar_t* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        length = wvsprintfW(data, fmt, args);
        va_end(args);
        assert(length < STR_SIZE);
    }

    void toUtf8(char* out, i32 outSize) {
        WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, data, -1, out, outSize, NULL, NULL);
    }
};

enum class FSEntryType: u8 {
    DIRECTORY=0,
    FILE=1
};

struct FileSystemEntry
{
    FSEntryType type;
    StrU<64> name;
    i64 size;
    // TODO: add icon

    inline bool isDir() {
        return type == FSEntryType::DIRECTORY;
    }
};

#define FS_MAX_ENTRIES 1024

struct AppWindow {

SDL_Window* window;
SDL_GLContext glContext;
bool running = true;
FileSystemEntry fsList[FS_MAX_ENTRIES];
i32 fsListCount = 0;
StrU<300> currentDir;

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

    currentDir.set(L"C:");
    listFsEntries(currentDir.data);
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

void listFsEntries(const wchar_t* path)
{
    StrU<300> search;
    search.setFmt(L"%s\\*", path);
    fsListCount = 0;

    LARGE_INTEGER li;
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search.data, &ffd);

    if(hFind == INVALID_HANDLE_VALUE) {
        LOG("ERROR> listFsEntries (%d)", GetLastError());
        return;
    }

    do {
        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            FileSystemEntry fse;
            fse.type = FSEntryType::DIRECTORY;
            fse.name.set(ffd.cFileName);
            fsList[fsListCount++] = fse;
        }
        else {
            FileSystemEntry fse;
            fse.type = FSEntryType::FILE;
            fse.name.set(ffd.cFileName);
            li.LowPart = ffd.nFileSizeLow;
            li.HighPart = ffd.nFileSizeHigh;
            fse.size = li.QuadPart;
            fsList[fsListCount++] = fse;
        }
    } while(FindNextFileW(hFind, &ffd) != 0);

    DWORD dwError = GetLastError();
    if(dwError != ERROR_NO_MORE_FILES)  {
        LOG("ERROR> listFsEntries (%d)", dwError);
    }

    FindClose(hFind);
}

void doUI()
{
    ImGui::ShowDemoWindow();

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH/2,WINDOW_HEIGHT));
    ImGui::Begin("##left_window", nullptr,
                 ImGuiWindowFlags_NoMove|
                 ImGuiWindowFlags_NoTitleBar|
                 ImGuiWindowFlags_NoResize);

    char curDirUtf8[600];
    currentDir.toUtf8(curDirUtf8, sizeof(curDirUtf8));
    ImGui::TextUnformatted(curDirUtf8);

    ImGui::BeginChild("##fs_item_list", ImVec2(-1, -1));


    char name[256];
    for(i32 i = 0; i < fsListCount; ++i) {
        fsList[i].name.toUtf8(name, 256);
        if(ImGui::Button(name) && fsList[i].isDir()) {
            currentDir.setFmt(L"%s\\%s", currentDir, fsList[i].name);
            listFsEntries(currentDir.data);
        }
    }

    ImGui::EndChild();

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

    AppWindow app;
    if(!app.init()) {
        return 1;
    }
    app.run();

    SDL_Quit();
    return 0;
}
