#ifndef UNICODE
    #define UNICODE
#endif

#ifndef _CRTDBG_MAP_ALLOC
    #error "def this shit"
#endif

#include <crtdbg.h>
#include <windows.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include "imgui/gl3w.h"
#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include "base.h"
#include "utils.h"
#include "icon_atlas.h"

// TODO: use 0xed window setup
// TODO: fetch file/dir user access

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900
#define MAX_TABS 64
#define MAX_PANELS 2

struct UiFseListOut
{
    i32 itemCount = 0;
    u8* selected = nullptr;
    i32 lastSelectedId = 0;
    i32 gotoItemId = -1;
    bool dragging = false;

    inline void clearSelection() {
        memset(selected, 0, sizeof(selected[0]) * itemCount);
    }

    inline void shiftSelection(i32 to) {
        assert(to >= 0 && to < itemCount);
        i32 start = min(lastSelectedId, to);
        i32 end = max(lastSelectedId, to);
        memset(&selected[start], 1, sizeof(selected[0]) * (end - start + 1));
    }
};

struct AppWindow {

#ifdef _WIN32
HWND hWindowHandle;
#endif

SDL_Window* sdl2Win;
SDL_GLContext glContext;
bool running = true;
Array<FileSystemEntry> tabFseList[MAX_TABS];
Path tabCurrentDir[MAX_TABS];
StrU<64> tabName[MAX_TABS];
char tabNameUtf8[MAX_TABS][128];
Array<u8> tabItemSelected[MAX_TABS];
i32 tabLastSelectedId[MAX_TABS] = {0};
i32 panelTabIdList[MAX_PANELS][MAX_TABS];
i32 panelTabSelected[MAX_PANELS] = {0};
i32 panelTabCount[MAX_PANELS] = {0};
const i32 panelCount = MAX_PANELS;
i32 tabCount = panelCount;

IconAtlas iconAtlas;

bool init()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    sdl2Win = SDL_CreateWindow("Fuldorz",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT,
                              SDL_WINDOW_OPENGL);

    if(!sdl2Win) {
        LOG("ERROR: can't create SDL2 window (%s)",  SDL_GetError());
        return false;
    }

    glContext = SDL_GL_CreateContext(sdl2Win);
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

#ifdef _WIN32
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(sdl2Win, &wmInfo);
    hWindowHandle = wmInfo.info.win.window;
#endif

    ImGui_ImplSdlGL3_Init(sdl2Win, "fuld0rz_imgui.ini");
    ImGui::StyleColorsLight();

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahoma.ttf", 15);

    glClearColor(1, 1, 1, 1);

    if(!iconAtlas.loadSystemIcons(sdl2Win)) {
        return false;
    }

    //currentDir.set(L"C:");
    for(i32 t = 0; t < tabCount; ++t) {
        tabCurrentDir[t].set(L"C:\\Prog\\Projets\\Fuldorz");
        tabUpdateFileList(t);
    }

    for(i32 p = 0; p < panelCount; ++p) {
        panelTabCount[p] = 1;
        panelTabIdList[p][0] = p;
        panelTabSelected[p] = p;
    }

    return true;
}

void cleanup()
{
    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(sdl2Win);
}

void run()
{
    while(running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            processEvent(&event);
        }
        ImGui_ImplSdlGL3_NewFrame(sdl2Win);

        doUI();

        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        SDL_GL_SwapWindow(sdl2Win);
    }

    cleanup();
}

i32 addTab(const wchar_t* path)
{
    assert(tabCount < MAX_TABS);
    i32 tabId = tabCount++;
    tabCurrentDir[tabId].set(path);
    tabUpdateFileList(tabId);
    return tabId;
}

// TODO: clip outside of widget bounds tabs
void ImGui_Tabs(i32* selectedTabId, i32* tabIdList, const char tabName[][128], i32* tabCount)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    ImVec2 pos = window->DC.CursorPos;
    const ImGuiID id = window->GetID(tabIdList);

    constexpr f32 tabSpacing = 2.0f;
    constexpr f32 paddingH = 12.0f;
    constexpr f32 paddingV = 6.0f;
    const f32 tabHeight = CalcTextSize("TEST").y + paddingV * 2.0f;
    ImVec2 widgetSize(window->Rect().GetWidth(), tabHeight);
    ImRect widgetBB(pos, pos + widgetSize);

    ItemSize(widgetSize);
    if(!ItemAdd(widgetBB, id)) {
        return;
    }

    constexpr u32 bgColor = 0xffaaaaaa;
    RenderFrame(widgetBB.Min, widgetBB.Max, bgColor, false);

    f32 offX = 0.0f;
    const i32 count = *tabCount;
    for(i32 i = 0; i < count; ++i) {
        const i32 tabId = tabIdList[i];
        const ImVec2 textSize = CalcTextSize(tabName[tabId]);
        const ImVec2 tabSize = textSize + ImVec2(paddingH * 2, paddingV * 2);
        const ImRect bb(pos + ImVec2(offX,0), pos + tabSize + ImVec2(offX,0));

        offX += bb.GetWidth() + tabSpacing;
        u32 tabColor = 0xffcccccc;
        u32 textColor = 0xff2d2d2d;

        const ImGuiID butId = id + i;
        bool held = false;
        bool hovered = false;
        ButtonBehavior(bb, butId, &hovered, &held);

        if(held) {
            *selectedTabId = tabId;
        }

        if(*selectedTabId == tabId) {
            tabColor = 0xffffffff;
            textColor = 0xff000000;
        }
        else if(hovered) {
            tabColor = 0xffffc5a3;
            textColor = 0xff000000;
        }

        RenderFrame(bb.Min, bb.Max, tabColor, false);

        PushStyleColor(ImGuiCol_Text, textColor);
        RenderTextClipped(bb.Min, bb.Max, tabName[tabId], NULL,
                          &textSize, ImVec2(0.5, 0.5), &bb);
        PopStyleColor();
    }

    // plus button
    constexpr char* plus = "+";
    const ImVec2 textSize = CalcTextSize(plus);
    const ImVec2 tabSize = textSize + ImVec2(paddingH * 2, paddingV * 2);
    const ImRect bb(pos + ImVec2(offX,0), pos + tabSize + ImVec2(offX,0));

    const ImGuiID butId = id + count;
    bool held = false;
    bool hovered = false;
    const bool previouslyHeld = (g.ActiveId == butId);
    ButtonBehavior(bb, butId, &hovered, &held);

    u32 butColor = 0xffffffff;
    u32 textColor = 0xff000000;

    if(previouslyHeld) {
        i32 newTabId = addTab(L"C:");
        assert(*tabCount < MAX_TABS);
        tabIdList[(*tabCount)++] = newTabId;
    }

    if(held) {
        butColor =  0xffba5000;
        textColor = 0xffffffff;
    }
    else if(hovered) {
        butColor =  0xffff6e00;
        textColor = 0xffffffff;
    }

    RenderFrame(bb.Min, bb.Max, butColor, false);

    PushStyleColor(ImGuiCol_Text, textColor);
    RenderTextClipped(bb.Min, bb.Max, plus, NULL,
                      &textSize, ImVec2(0.5, 0.5), &bb);
    PopStyleColor();
}

void ImGui_Path(Path* path, i32 tabId)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 2));

    ImGui::BeginGroup();

    char buff[256];
    for(i32 f = 0; f < path->folderCount; ++f) {
        const i32 folderLen = path->folder[f].nameLen;
        toUtf8(path->folder[f].name, buff, sizeof(buff), folderLen);
        buff[folderLen] = 0;

        if(ImGui::Button(buff)) {
            path->goUp(path->folderCount - f - 1);
            tabUpdateFileList(tabId);
        }
        if(f+1 < path->folderCount) {
            ImGui::SameLine();
        }
    }

    ImGui::EndGroup();

    ImGui::PopStyleVar(1);
}

void ImGui_FseList(FileSystemEntry* list, const i32 listCount, UiFseListOut* out)
{
    i32 clickedId = -1;
    u8* selected = out->selected;
    out->itemCount = listCount;

    using namespace ImGui;
    BeginChild("##fs_item_list", ImVec2(-1, -1));

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindow* window = GetCurrentWindow();
    const f32 widgetWidth = GetContentRegionAvailWidth();
    const ImGuiStyle& style = GetStyle();
    const ImVec2 iconSize(16,16);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    //PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    // line wide button
    char name[64];
    char fileSizeStr[64];
    ImVec2 pos = window->DC.CursorPos;
    const ImVec2 lineSize(widgetWidth, GetTextLineHeight() + style.FramePadding.y * 2);
    const f32 iconOffY = lineSize.y * 0.5 - iconSize.y * 0.5;

    for(i32 i = 0; i < listCount; ++i) {
        const FileSystemEntry& entry = list[i];
        if(entry.isSpecial()) {
            continue;
        }
        ImRect bb(pos, pos + lineSize);
        ItemSize(lineSize);

        const ImGuiID butId = window->GetID(list + i);
        bool held = false;
        bool hovered = false;
        const bool previouslyHeld = (g.ActiveId == butId);
        if(ItemAdd(bb, butId)) {
            ButtonBehavior(bb, butId, &hovered, &held);
        }

        // TODO: implement drag and dropping
        if(held && !previouslyHeld) {
            if(!io.KeyCtrl) {
                out->clearSelection();
            }

            if(io.KeyShift) {
                out->shiftSelection(i);
            }
            else {
                if(io.KeyCtrl && selected[i]) {
                    selected[i] = false;
                }
                else {
                    selected[i] = true;
                }
            }

            out->lastSelectedId = i;
        }

        u32 frameColor = 0xffffffff;
        if(hovered) {
            frameColor = 0xffffe4bf;
        }
        if(held || selected[i]) {
            frameColor = 0xffffb775;
        }

        RenderFrame(bb.Min, bb.Max, frameColor, true);

        if(hovered && io.MouseDoubleClicked[0]) {
           clickedId = i;
        }

        entry.name.toUtf8(name, sizeof(name));
        const i32 iconId = entry.icon;
        ImTextureID iconAtlasTex = 0;
        i32 c, r;
        assert(iconId >= 0);
        assert(iconId < iconAtlas.sysImgListCount);
        iconAtlasTex = (ImTextureID)(i64)iconAtlas.bmSysImgList.gpuTexId;
        c = iconAtlas.aiSysImgList.columns;
        r = iconAtlas.aiSysImgList.rows;

        ImVec2 uv0((iconId % c) / (f32)c, (iconId / c) / (f32)r);
        ImVec2 uv1(((iconId % c) + 1) / (f32)c, ((iconId / c) + 1) / (f32)r);

        u32 textColor = 0xff000000;
        if(entry.isHidden()) {
            textColor = 0xff707070;
        }

        // icon and filename
        ImVec2 iconPos = pos + ImVec2(style.FramePadding.x, iconOffY);
        ImVec2 namePos = ImVec2(iconPos.x + iconSize.x + 2, pos.y + style.FramePadding.y);

        draw_list->AddImage(iconAtlasTex, iconPos, iconPos + iconSize, uv0, uv1);
        draw_list->AddText(namePos, textColor, name);

        // file size
        if(entry.isFile()) {
            const i64 entrySize = entry.size;
            if(entrySize < (1024*1024)) {
                sprintf(fileSizeStr, "%lld Kb", entrySize/1024);
            }
            else {
                sprintf(fileSizeStr, "%lld Mb", entrySize/(1024*1024));
            }
            const ImVec2 entrySizeStrSize = CalcTextSize(fileSizeStr);
            ImVec2 sizePos = pos + ImVec2(lineSize.x - entrySizeStrSize.x - style.FramePadding.x,
                                          style.FramePadding.y);
            draw_list->AddText(sizePos, textColor, fileSizeStr);
        }

        pos.y += lineSize.y;
    }

    //PopStyleVar(1);
    EndChild();

    out->gotoItemId = clickedId;
    return;
}

void ui_tabContent(i32 tabId)
{
    ImGui_Path(&tabCurrentDir[tabId], tabId);

    UiFseListOut fseListOut = {};
    fseListOut.selected = tabItemSelected[tabId].data();
    fseListOut.lastSelectedId = tabLastSelectedId[tabId];

    ImGui_FseList(tabFseList[tabId].data(), tabFseList[tabId].size(), &fseListOut);

    tabLastSelectedId[tabId] = fseListOut.lastSelectedId;

    if(fseListOut.gotoItemId != -1) {
        const FileSystemEntry& entry = tabFseList[tabId][fseListOut.gotoItemId];
        if(entry.isDir()) {
            tabCurrentDir[tabId].goDown(entry.name.data);
            tabUpdateFileList(tabId);
        }
        else if(entry.isFile()) {
            StrU<600> filepath = tabCurrentDir[tabId].str;
            filepath.append(L"\\");
            filepath.append(entry.name.data);

            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.hwnd = hWindowHandle;
            sei.fMask = SEE_MASK_INVOKEIDLIST;
            sei.nShow = SW_SHOWNORMAL;
            sei.lpVerb = NULL;
            sei.lpFile = filepath.data;

            if(!ShellExecuteExW(&sei)) {
                LOG("ERROR> ShellExecuteExW(%ls) [%d]", filepath.data, GetLastError());
            }
        }
    }
}

void ui_debug()
{
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

void ImGui_PanelFocusedIndicator(i32* focusedId, i32 thisId, u32 colFocus, u32 colNoFocus)
{
    ImGuiIO& io = ImGui::GetIO();
    constexpr f32 border = 3.0f;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImRect& r = window->Rect();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

    ImGui::ItemSize(ImVec2(r.GetWidth(), border));

    if(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow|ImGuiFocusedFlags_ChildWindows) &&
            io.MouseDown[0] && r.Contains(io.MousePos)) {
        *focusedId = thisId;
    }

    u32 col = colNoFocus;
    if(*focusedId == thisId) {
        col = colFocus;
    }

    ImGui::RenderFrame(r.Min, ImVec2(r.Max.x, border), col, false, 0);

    ImGui::PopStyleVar(1);
}

void doUI()
{
    ImGui::ShowDemoWindow();
    static i32 windowFocusedId = 0;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));

    f32 offX = 0;
    const f32 panelWidth = WINDOW_WIDTH/(f32)panelCount;
    char windowStrId[64];
    for(i32 p = 0; p < panelCount; ++p) {
        sprintf(windowStrId, "##window%d", p);

        ImGui::SetNextWindowPos(ImVec2(offX, 0));
        ImGui::SetNextWindowSize(ImVec2(panelWidth, WINDOW_HEIGHT));
        ImGui::Begin(windowStrId, nullptr,
                     ImGuiWindowFlags_NoMove|
                     ImGuiWindowFlags_NoTitleBar|
                     ImGuiWindowFlags_NoResize|
                     ImGuiWindowFlags_NoBringToFrontOnFocus);


        ImGui_PanelFocusedIndicator(&windowFocusedId, p, 0xff0000ff, 0xffaaaaaa);
        ImGui_Tabs(&panelTabSelected[p], panelTabIdList[p], tabNameUtf8, &panelTabCount[p]);
        ui_tabContent(panelTabSelected[p]);

        ImGui::End();
        offX += panelWidth;
    }

    ImGui::PopStyleVar(2);

    ui_debug();
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
            tabCurrentDir[0].goUp();
            tabUpdateFileList(0);
            return;
        }
    }
}

void tabUpdateFileList(i32 tabId)
{
    bool r = listFsEntries(tabCurrentDir[tabId].getStr(), &(tabFseList[tabId]));
    if(!r) {
        tabCurrentDir[tabId].goUp(1);
        return;
    }

    // update tab name
    tabName[tabId].set(tabCurrentDir[tabId].getLastFolder().name,
                       tabCurrentDir[tabId].getLastFolder().nameLen);
    tabName[tabId].toUtf8(tabNameUtf8[tabId], sizeof(tabNameUtf8[tabId]));

    // update system image list if needed
    const i32 fseCount = tabFseList[tabId].size();
    const FileSystemEntry* fseList = tabFseList[tabId].data();
    bool foundOne = false;
    for(i32 i = 0; i < fseCount; ++i) {
        if(fseList[i].icon >= iconAtlas.sysImgListCount) {
            foundOne = true;
            break;
        }
    }

    if(foundOne) {
        iconAtlas.updateSystemImageList();
    }

    // sort entries by sort criteria
    sortFse(tabFseList[tabId].data(), tabFseList[tabId].size(), 0);

    // reset selection
    tabItemSelected[tabId].resize(fseCount);
    u8* selected = tabItemSelected[tabId].data();
    memset(selected, 0, sizeof(selected[0]) * fseCount);
}

};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        LOG("SDL Error: %s", SDL_GetError());
        return 1;
    }

    // TODO: change this to multi threaded (CoInitializeEx)
    /*if(CoInitialize(NULL) != S_OK) {
        return 1;
    }*/
    if(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) != S_OK) {
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
