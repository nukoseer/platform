#pragma once

#include "types.h"

#undef assert
#define assert(x) do { if (!(x)) { __debugbreak(); } } while (0)

#define array_count(x) (sizeof(x) / sizeof(*(x)))

#define stringfy_(x) #x
#define stringfy(x) stringfy_(x)

#define KIBIBYTES(x) ((x) * (1024ULL))
#define MIBIBYTES(x) ((KIBIBYTES(x)) * (1024ULL))
#define GIBIBYTES(x) ((MIBIBYTES(x)) * (1024ULL))
#define TIBIBYTES(x) ((GIBIBYTES(x)) * (1024ULL))

