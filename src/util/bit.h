#ifndef UTIL_BIT_H
#define UTIL_BIT_H

#include <stdint.h>
#include "debug.h"
#include "macros.h"

#define U64(x) (UINT64_C(x))
#define U32(x) (UINT32_C(x))

#if GNUC_AT_LEAST(3, 4)
#define USE_BUILTIN(fn, arg) \
    if (__builtin_types_compatible_p(__typeof__(arg), unsigned long long)) { \
        return __builtin_ ## fn ## ll(arg); \
    } else if (__builtin_types_compatible_p(__typeof__(arg), unsigned long)) { \
        return __builtin_ ## fn ## l(arg); \
    } else if (__builtin_types_compatible_p(__typeof__(arg), unsigned int)) { \
        return __builtin_ ## fn(arg); \
    }
#else
#define USE_BUILTIN(fn, arg)
#endif

static inline unsigned int u64_popcount(uint64_t n)
{
    USE_BUILTIN(popcount, n);
    n -= ((n >> 1) & U64(0x5555555555555555));
    n = (n & U64(0x3333333333333333)) + ((n >> 2) & U64(0x3333333333333333));
    n = (n + (n >> 4)) & U64(0x0F0F0F0F0F0F0F0F);
    return (n * U64(0x0101010101010101)) >> 56;
}

static inline unsigned int u32_popcount(uint32_t n)
{
    USE_BUILTIN(popcount, n);
    n -= ((n >> 1) & U32(0x55555555));
    n = (n & U32(0x33333333)) + ((n >> 2) & U32(0x33333333));
    n = (n + (n >> 4)) & U32(0x0F0F0F0F);
    return (n * U32(0x01010101)) >> 24;
}

#endif
