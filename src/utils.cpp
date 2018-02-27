#include "utils.h"
#include <stdio.h>

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
                                      SHGFI_ICONLOCATION  | SHGFI_USEFILEATTRIBUTES);
        if(SUCCEEDED(hr)) {
            char pathUtf8[600];
            char display[256];
            filePath.toUtf8(pathUtf8, sizeof(pathUtf8));
            StrU<256> displayW;
            displayW.set(sfi.szDisplayName);
            displayW.toUtf8(display, sizeof(display));
            LOG("%s icon=%llx icondId=%d iconFileName=%s", pathUtf8, (i64)sfi.hIcon, sfi.iIcon,
                 display);
        }

        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            FileSystemEntry fse;
            fse.type = FSEntryType::DIRECTORY;
            fse.name.set(ffd.cFileName);
            entries[(*entryCount)++] = fse;
        }
        else {
            FileSystemEntry fse;
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
