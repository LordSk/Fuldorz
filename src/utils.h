#pragma once
#include "base.h"
#include <windows.h>
#include <assert.h>

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
    HICON icon;

    inline bool isDir() {
        return type == FSEntryType::DIRECTORY;
    }
};

void listFsEntries(const wchar_t* path, FileSystemEntry* entries, i32* entryCount);
