#include "utils.h"
#include <stdio.h>
#include <crtdbg.h>

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
    assert(folderCount > 0);
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
    assert(folderCount < PATH_MAX_FOLDERS);
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

bool listFsEntries(const wchar_t* path, Array<FileSystemEntry>* entries)
{
    StrU<300> search;
    search.setFmt(L"%s\\*", path);

    LARGE_INTEGER li;
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search.data, &ffd);

    if(hFind == INVALID_HANDLE_VALUE) {
        LOG("ERROR> listFsEntries (%d)", GetLastError());
        return false;
    }

    entries->clear();

    do {
        StrU<600> filePath;
        const wchar_t* filename = ffd.cFileName;
        if(wcslen(ffd.cFileName) > 32) {
            filename = ffd.cAlternateFileName;
        }

        filePath.setFmt(L"%s\\%s", path, filename);
        SHFILEINFOW sfi = {0};
#if 0
        {
            SHFILEINFOW sfi2 = {0};
            DWORD_PTR hr = SHGetFileInfoW(filePath.data, ffd.dwFileAttributes, &sfi2, sizeof(sfi2),
                                          SHGFI_ICONLOCATION | SHIL_SYSSMALL | SHGFI_USEFILEATTRIBUTES);
            LOGU("#1 [%s] iconId=%d iconSrc=%s", filename, sfi.iIcon, sfi.szDisplayName);
        }
#endif
        DWORD_PTR hr = SHGetFileInfoW(filePath.data, ffd.dwFileAttributes, &sfi, sizeof(sfi),
                                SHGFI_SYSICONINDEX | SHIL_SYSSMALL | SHGFI_USEFILEATTRIBUTES);
        //LOGU("#2 [%s] iconId=%d iconSrc=%s %llx", filename, sfi.iIcon, sfi.szDisplayName, hr);

        FileSystemEntry fse = {};
        if(hr != 0) {
            fse.icon = sfi.iIcon;
        }
        else {
            fse.icon = 0;
            assert(0);
        }

        assert(fse.icon >= 0);

        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
            fse.attributes |= FATT_HIDDEN;
        }
        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
            fse.attributes |= FATT_READ_ONLY;
        }
        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
            fse.attributes |= FATT_SYSTEM;
        }
        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            fse.attributes |= FATT_LINK;
        }

        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if(wcscmp(filename, L".") == 0 || wcscmp(filename, L"..") == 0) {
                fse.type = FSEntryType::FSETYPE_SPECIAL_DIR;
            }
            else {
                fse.type = FSEntryType::FSETYPE_DIRECTORY;
            }

            fse.name.set(filename);
            fse.size = 0;
        }
        else {
            fse.type = FSEntryType::FSETYPE_FILE;
            fse.name.set(filename);
            li.LowPart = ffd.nFileSizeLow;
            li.HighPart = ffd.nFileSizeHigh;
            fse.size = li.QuadPart;
        }
        entries->push_back(fse);
        /*assert(entries->data()[entries->count()-1].icon == fse.icon);
        assert(entries->data()[entries->count()-1].name.length == fse.name.length);
        assert(entries->data()[entries->count()-1].type == fse.type);*/
    } while(FindNextFileW(hFind, &ffd) != 0);

    DWORD dwError = GetLastError();
    if(dwError != ERROR_NO_MORE_FILES)  {
        LOG("ERROR> listFsEntries (%d)", dwError);
        return false;
    }

    FindClose(hFind);
    return true;
}

i32 compareFse(const void* a, const void* b)
{
    const FileSystemEntry& fa = *(FileSystemEntry*)a;
    const FileSystemEntry& fb = *(FileSystemEntry*)b;
    if(fa.type < fb.type) return -1;
    if(fa.type > fb.type) return 1;
    return 0;
}

void sortFse(FileSystemEntry* entries, const i32 entryCount, i32 sortThing)
{
    qsort(entries, entryCount, sizeof(FileSystemEntry), compareFse);

#if 0
    // bubble sort
    bool sorting = true;
    while(sorting) {
        sorting = false;
        for(i32 i = 1; i < entryCount; ++i) {
            //assert((i32)entries[i-1].type > 0 && (i32)entries[i-1].type < 4);
            //if(compareFse(&entries[i], &entries[i-1]) < 0) {
            if(entries[i].type < entries[i-1].type) {
                FileSystemEntry temp = entries[i-1]; // swap
                entries[i-1] = entries[i];
                entries[i] = temp;
                sorting = true;
            }
        }
    }
#endif
}

bool renameFse(const Path& dir, FileSystemEntry* entry, const wchar_t* newName)
{
    SHFILEOPSTRUCTW fileOp;
    fileOp.hwnd = NULL;
    fileOp.wFunc = FO_RENAME;

    StrU<600> from = dir.str;
    from.append(L"\\");
    from.append(entry->name);
    from.append(L"\0"); // double null terminated

    fileOp.pFrom = from.data;

    StrU<600> to = dir.str;
    to.append(L"\\");
    to.append(newName);
    to.append(L"\0"); // double null terminated

    fileOp.pTo = to.data;

    fileOp.fFlags = FOF_ALLOWUNDO;

    LOG("FileOp> renaming %ls to %ls", fileOp.pFrom, fileOp.pTo);
    i32 r = SHFileOperationW(&fileOp);
    bool success = (r == 0 && !fileOp.fAnyOperationsAborted);
    if(success) {
        entry->name.set(newName);
    }

    return success;
}
