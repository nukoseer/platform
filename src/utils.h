#pragma once

#include "types.h"

#undef assert
#define assert(x) do { if (!(x)) { __debugbreak(); } } while (0)

#define array_count(x) (sizeof(x) / sizeof(*(x)))

#define stringfy_(x) #x
#define stringfy(x) stringfy_(x)

#undef offsetof
#define offsetof(type, member) ((usize)&(((type*)0)->member))

#define concat_(x, y) x##y
#define concat(x, y) concat_(x, y)

#define unique(name) concat(name, __LINE__)
#define defer_loop(begin, end) for (u32 unique(_i_) = ((begin), 0); unique(_i_) == 0; (++unique(_i_), (end)))
#define defer(x) defer_loop(0, x)


#define KIBIBYTES(x) ((x) * (1024ULL))
#define MIBIBYTES(x) ((KIBIBYTES(x)) * (1024ULL))
#define GIBIBYTES(x) ((MIBIBYTES(x)) * (1024ULL))
#define TIBIBYTES(x) ((GIBIBYTES(x)) * (1024ULL))

static inline i32 string_find_leading_char(const char* string, i32 offset, i32 length, char c)
{
    i32 index = -1;

    for (i32 i = offset; i < offset + length; ++i)
    {
        if (string[i] == c)
        {
            index = i;
            break;
        }
    }

    return index;
}
