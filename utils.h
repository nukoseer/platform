#pragma once

#include "types.h"

#undef assert
#define assert(x) do { if (!(x)) { __debugbreak(); } } while (0)

#define array_count(x) (sizeof(x) / sizeof(*(x)))

#define stringfy_(x) #x
#define stringfy(x) stringfy_(x)

