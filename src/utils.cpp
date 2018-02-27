#include "utils.h"
#include <stdio.h>

void Path::set(const wchar_t *pathStr)
{
    levels = 1;
    str.set(pathStr);
    const wchar_t* data = str.data;
    const i32 len = str.length;
    assert(data[len-1] != '\\');

    i32 cur = 0;
    while(cur < len) {
        if(data[cur] == '\\') {
            levels++;
        }
        cur++;
    }
}

void Path::goUp(i32 lvls)
{
    levels -= lvls;
    while(lvls--) {
        i32 len = str.length;
        wchar_t* data = str.data;
        while(len && data[len] != '\\') {
            len--;
        }
        if(len > 0) { // found, apply
            str.length = len;
            data[len] = 0;
        }
    }
}

void Path::goDown(const wchar_t* folder)
{
    str.append(L"\\");
    str.append(folder);
    levels++;
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
        LOGU("%s iconId=%d iconSrc=%s", ffd.cFileName, sfi.iIcon, sfi.szDisplayName);
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
