#pragma once
#include <stdint.h>

#define LOG(fmt, ...) (printf(fmt"\n", ##__VA_ARGS__), fflush(stdout))
#define LOGU(fmt, ...) (wprintf(fmt L"\n", ##__VA_ARGS__), fflush(stdout))
#define arr_count(arr) (sizeof(arr)/sizeof(arr[0]))

typedef uint8_t u8;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef float f32;

#define I32_MAX 0x7FFFFFFF
