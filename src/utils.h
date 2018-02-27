#pragma once
#include "base.h"
#include <windows.h>
#include <assert.h>

inline void toUtf8(const wchar_t* src, char* out, i32 outSize, i32 srcLen = -1)
{
    WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, src, srcLen, out, outSize, NULL, NULL);
}

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

    void append(const wchar_t* src) {
        i32 len = wcslen(src);
        assert(length + len < STR_SIZE);
        memmove(data + length, src, len * sizeof(data[0]));
        length += len;
        data[length] = 0;
    }
};

struct Path
{
    StrU<600> str;
    i32 levels = 1;

    void set(const wchar_t* pathStr);
    void goUp(i32 levels = 1);
    void goDown(const wchar_t* folder);
    inline const wchar_t* getStr() {
        return str.data;
    }
};

enum class FSEntryType: u8 {
    DIRECTORY=0,
    SPECIAL_DIR,
    FILE
};

struct FileSystemEntry
{
    FSEntryType type;
    StrU<64> name;
    i64 size;
    i32 icon;

    inline bool isSpecial() {
        return type == FSEntryType::SPECIAL_DIR;
    }
    inline bool isDir() {
        return type == FSEntryType::DIRECTORY;
    }
};

void listFsEntries(const wchar_t* path, FileSystemEntry* entries, i32* entryCount);
