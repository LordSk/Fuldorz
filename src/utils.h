#pragma once
#include "base.h"
#include <windows.h>
#include <assert.h>

#ifdef _WIN32
    #define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
    #define aligned_free(ptr) _aligned_free(ptr)
#endif

#if 1
template<typename T>
struct Array
{
    T _stack[8] alignas(T);
    i32 _count = 0;
    T* _data = _stack;
    i32 _capacity = arr_count(_stack);

    ~Array() {
        if(_data != _stack) {
            aligned_free(_data);
            _data = nullptr;
        }
    }

    // std::vector aliases
    //-------------------------------
    inline void push_back(T elem) {
        push(elem);
    }

    inline i32 size() const {
        return _count;
    }

    inline void resize(i32 elemCount) {
        if(elemCount > _capacity) {
            i32 newCap = max(elemCount, _capacity * 2);
            reserve(newCap);
        }
        _count = elemCount;
    }
    //-------------------------------

    inline void push(T elem) {
        if(_count+1 > _capacity) {
            reserve(_capacity * 2);
        }
        _data[_count++] = elem;
    }

    inline void reserve(i32 newCapacity) {
        newCapacity = max(16, newCapacity);
        if(_data == _stack) {
            _data = (T*)aligned_alloc(alignof(T), sizeof(T) * newCapacity);
            assert(_data);
            memmove(_data, _stack, sizeof(_stack));
        }
        else {
            T* newData = (T*)aligned_alloc(alignof(T), sizeof(T) * newCapacity);
            assert(newData);
            if(_data) {
                memmove(newData, _data, sizeof(T) * _count);
                aligned_free(_data);
            }
            _data = newData;

        }
        _capacity = newCapacity;
    }

    inline void clear() {
        _count = 0;
    }

    inline T* const data() {
        return _data;
    }

    inline const T* data() const {
        return _data;
    }

    inline i32 count() const {
        return _count;
    }

    inline i32 capacity() const {
        return _capacity;
    }

    inline T& operator[](i32 id) {
        assert(id >= 0 && id < _count);
        return _data[id];
    }
};
#else
#include <vector>
#define Array std::vector
#endif

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
    i32 length = 0;

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

    template<i32 OTHER_SIZE>
    void append(const StrU<OTHER_SIZE>& other) {
        const i32 len = other.length;
        assert(length + len < STR_SIZE);
        memmove(data + length, other.data, len * sizeof(data[0]));
        length += len;
        data[length] = 0;
    }

    void appendf(const wchar_t* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        length += wvsprintfW(data + length, fmt, args);
        va_end(args);
        assert(length < STR_SIZE);
        data[length] = 0;
    }

    bool equals(const wchar_t* str) const {
        const i32 len = wcslen(src);
        if(len != length) return false;
        for(i32 i = 0; i < len; ++i) {
            if(data[i] != str[i]) {
                return false;
            }
        }
        return true;
    }

    template<i32 OTHER_SIZE>
    bool equals(const StrU<OTHER_SIZE>& other) const {
        const i32 len = other.length;
        const wchar_t* str = other.data;
        if(len != length) return false;
        for(i32 i = 0; i < len; ++i) {
            if(data[i] != str[i]) {
                return false;
            }
        }
        return true;
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

enum FSEntryType: i32 {
    FSETYPE_INVALID=0,
    FSETYPE_DIRECTORY,
    FSETYPE_SPECIAL_DIR,
    FSETYPE_FILE
};

enum: u32 {
    FATT_NONE      = 0,
    FATT_HIDDEN    = 0x1,
    FATT_READ_ONLY = 0x2,
    FATT_SYSTEM    = 0x4,
    FATT_LINK      = 0x8,
};

struct FileSystemEntry
{
    FSEntryType type = FSETYPE_INVALID;
    StrU<64> name = {};
    i64 size = 0;
    i32 icon = 0;
    u32 attributes = 0;

    inline bool isSpecial() const {
        return type == FSEntryType::FSETYPE_SPECIAL_DIR;
    }

    inline bool isDir() const {
        return type == FSEntryType::FSETYPE_DIRECTORY;
    }

    inline bool isFile() const {
        return type == FSEntryType::FSETYPE_FILE;
    }

    inline bool isHidden() const {
        return attributes & FATT_HIDDEN;
    }
};

bool listFsEntries(const wchar_t* path, Array<FileSystemEntry> *entries);
void sortFse(FileSystemEntry* entries, const i32 entryCount, i32 sortThing);
bool renameFse(const Path &dir, FileSystemEntry *entry, const wchar_t* newName);
