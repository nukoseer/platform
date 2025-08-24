#pragma once

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef uintptr_t uptr;

typedef size_t usize;

#undef bool
typedef int32_t bool;

#undef true
#undef false

#define true  1
#define false 0

// NOTE: This is for HLSL shader output. We should not need this at the end probably.
#define BYTE uint8_t
