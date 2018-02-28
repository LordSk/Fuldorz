#include "utils.h"
#include <stdio.h>

void Path::set(const wchar_t *pathStr)
{
    str.set(pathStr);
    const wchar_t* data = str.data;
    const i32 len = str.length;
    assert(data[len-1] != '\\');

    folderCount = 1;
    i32 curFolder = 0;
    folder[curFolder].name = str.data;

    i32 cur = 0;
    while(cur < len) {
        if(data[cur] == '\\') {
            folder[curFolder].nameLen = &data[cur] - folder[curFolder].name;
            if(cur + 1 < len) {
                curFolder = folderCount++;
                folder[curFolder].name = &data[cur+1];
            }
        }
        cur++;
    }

    folder[curFolder].nameLen = &data[cur] - folder[curFolder].name;
    if(folder[curFolder].nameLen == 0) {
        folderCount--;
    }

#if 0
    for(i32 i = 0; i < folderCount; ++i) {
        LOGU("%d: %.*s", i, folder[i].nameLen, folder[i].name);
    }
#endif
}

void Path::goUp(i32 lvls)
{
    if(lvls <= 0 || folderCount == 1) {
        return;
    }

    folderCount -= lvls;
    str.length = folder[folderCount-1].name + folder[folderCount-1].nameLen - str.data;
    str.data[str.length] = 0;

#if 0
    for(i32 i = 0; i < folderCount; ++i) {
        LOGU("%d: %.*s", i, folder[i].nameLen, folder[i].name);
    }
#endif
}

void Path::goDown(const wchar_t* folderStr)
{
    i32 f = folderCount++;
    folder[f].name = str.data + str.length + 1;

    str.append(L"\\");
    str.append(folderStr);

    folder[f].nameLen = str.data + str.length - folder[f].name;

#if 0
    for(i32 i = 0; i < folderCount; ++i) {
        LOGU("%d: %.*s", i, folder[i].nameLen, folder[i].name);
    }
#endif
}

void listFsEntries(const wchar_t* path, FileSystemEntry* entries, i32* entryCount)
{
    StrU<300> search;
    search.setFmt(L"%s\\*", path);

    LARGE_INTEGER li;
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search.data, &ffd);

    if(hFind == INVALID_HANDLE_VALUE) {
        LOG("ERROR> listFsEntries (%d)", GetLastError());
        return;
    }

    *entryCount = 0;

    do {
        StrU<600> filePath;
        filePath.setFmt(L"%s\\%s", path, ffd.cFileName);
        SHFILEINFOW sfi = {0};
        DWORD_PTR hr = SHGetFileInfoW(filePath.data, ffd.dwFileAttributes, &sfi, sizeof(sfi),
                                      SHGFI_ICONLOCATION|SHGFI_USEFILEATTRIBUTES);
        //LOGU("%s iconId=%d iconSrc=%s", ffd.cFileName, sfi.iIcon, sfi.szDisplayName);
        FileSystemEntry fse = {};
        if(SUCCEEDED(hr)) {
            fse.icon = sfi.iIcon;
#if 0
            char pathUtf8[600];
            char display[256];
            filePath.toUtf8(pathUtf8, sizeof(pathUtf8));
            StrU<256> displayW;
            displayW.set(sfi.szDisplayName);
            displayW.toUtf8(display, sizeof(display));
            LOG("%s icon=%llx icondId=%d iconFileName=%s", pathUtf8, (i64)sfi.hIcon, sfi.iIcon,
                 display);
#endif
        }

        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if(wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) {
                fse.type = FSEntryType::SPECIAL_DIR;
            }
            else {
                fse.type = FSEntryType::DIRECTORY;
            }

            fse.name.set(ffd.cFileName);
            entries[(*entryCount)++] = fse;
        }
        else {
            fse.type = FSEntryType::FILE;
            fse.name.set(ffd.cFileName);
            li.LowPart = ffd.nFileSizeLow;
            li.HighPart = ffd.nFileSizeHigh;
            fse.size = li.QuadPart;
            entries[(*entryCount)++] = fse;
        }
    } while(FindNextFileW(hFind, &ffd) != 0);

    DWORD dwError = GetLastError();
    if(dwError != ERROR_NO_MORE_FILES)  {
        LOG("ERROR> listFsEntries (%d)", dwError);
    }

    FindClose(hFind);
}
