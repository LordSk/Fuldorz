#pragma once
#include <stdint.h>

#define LOG(fmt, ...) (printf(fmt"\n", ##__VA_ARGS__))
#define LOGU(fmt, ...) (wprintf(fmt L"\n", ##__VA_ARGS__))

typedef uint8_t u8;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef float f32;
