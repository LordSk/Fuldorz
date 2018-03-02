#pragma once
#include "base.h"
#include <windows.h>
#include <assert.h>

template<typename T>
struct Array
{
    T _stack[8];
    T* _data = _stack;
    i32 _count = 0;
    i32 _capacity = arr_count(_stack);

    inline void push(T elem) {
        if(_count+1 > _capacity) {
            reserve(_capacity * 2);
        }
        _data[_count++] = elem;
    }

    inline void reserve(i32 newCapacity) {
        if(_data == _stack) {
            _data = (T*)malloc(sizeof(T) * newCapacity);
            memmove(_data, _stack, sizeof(_stack));
        }
        else {
            _data = (T*)realloc(_data, sizeof(T) * newCapacity);
        }
        _capacity = newCapacity;
    }

    inline void clear() {
        _count = 0;
    }

    inline T* data() {
        return _data;
    }

    inline i32 count() const {
        return _count;
    }

    inline T& operator[](i32 id) {
        assert(id >= 0 && id < _count);
        return _data[id];
    }
};

inline void toUtf8(const wchar_t* src, char* out, i32 outSize, i32 srcLen = -1)
{
    i32 written = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, src,
                                      srcLen, out, outSize, NULL, NULL);
    out[written] = 0;
}

template<u32 STR_SIZE>
struct StrU
{
    wchar_t data[STR_SIZE];
    i32 length;

    void set(const wchar_t* src, i32 len = -1) {
        if(len < 0) {
            length = wcslen(src);
        }
        else {
           length = len;
        }
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
        data[length] = 0;
    }

    void toUtf8(char* out, i32 outSize) const {
        i32 written = WideCharToMultiByte(CP_UTF8, WC_NO_BEST_FIT_CHARS, data,
                                          -1, out, outSize, NULL, NULL);
        out[written] = 0;
    }

    void append(const wchar_t* src) {
        i32 len = wcslen(src);
        assert(length + len < STR_SIZE);
        memmove(data + length, src, len * sizeof(data[0]));
        length += len;
        data[length] = 0;
    }
};

#define PATH_MAX_FOLDERS 32

struct Path
{
    StrU<600> str;

    struct Folder
    {
        const wchar_t* name = nullptr;
        i32 nameLen = 0;
    }
    folder[PATH_MAX_FOLDERS];
    i32 folderCount = 0;

    void set(const wchar_t* pathStr);
    void goUp(i32 levels = 1);
    void goDown(const wchar_t* folderStr);

    inline const wchar_t* getStr() {
        return str.data;
    }

    inline const Folder& getLastFolder() const {
        return folder[folderCount-1];
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

    inline bool isSpecial() const {
        return type == FSEntryType::SPECIAL_DIR;
    }
    inline bool isDir() const {
        return type == FSEntryType::DIRECTORY;
    }
};

bool listFsEntries(const wchar_t* path, Array<FileSystemEntry> *entries);
