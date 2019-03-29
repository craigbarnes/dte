#ifndef UTIL_BIT_H
#define UTIL_BIT_H

#include <stdint.h>
#include "macros.h"
#include "../debug.h"

#define U64 UINT64_C
#define U32 UINT32_C

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

static inline unsigned int bit_popcount_u64(uint64_t n)
{
    USE_BUILTIN(popcount, n);
    n -= ((n >> 1) & U64(0x5555555555555555));
    n = (n & U64(0x3333333333333333)) + ((n >> 2) & U64(0x3333333333333333));
    n = (n + (n >> 4)) & U64(0x0F0F0F0F0F0F0F0F);
    return (n * U64(0x0101010101010101)) >> 56;
}

static inline unsigned int bit_popcount_u32(uint32_t n)
{
    USE_BUILTIN(popcount, n);
    n -= ((n >> 1) & U32(0x55555555));
    n = (n & U32(0x33333333)) + ((n >> 2) & U32(0x33333333));
    n = (n + (n >> 4)) & U32(0x0F0F0F0F);
    return (n * U32(0x01010101)) >> 24;
}

static inline unsigned int bit_count_leading_zeros_u64(uint64_t n)
{
    BUG_ON(n == 0);
    USE_BUILTIN(clz, n);
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
    n |= (n >> 32);
    return bit_popcount_u64(~n);
}

static inline unsigned int bit_count_leading_zeros_u32(uint32_t n)
{
    BUG_ON(n == 0);
    USE_BUILTIN(clz, n);
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
    return bit_popcount_u32(~n);
}

static inline unsigned int bit_count_trailing_zeros_u64(uint64_t n)
{
    BUG_ON(n == 0);
    USE_BUILTIN(ctz, n);
    return bit_popcount_u64(~n & (n - 1));
}

static inline unsigned int bit_count_trailing_zeros_u32(uint32_t n)
{
    BUG_ON(n == 0);
    USE_BUILTIN(ctz, n);
    return bit_popcount_u32(~n & (n - 1));
}

static inline unsigned int bit_find_first_set_u64(uint64_t n)
{
    USE_BUILTIN(ffs, n);
    return n ? bit_count_trailing_zeros_u64(n) + 1 : 0;
}

static inline unsigned int bit_find_first_set_u32(uint32_t n)
{
    USE_BUILTIN(ffs, n);
    return n ? bit_count_trailing_zeros_u32(n) + 1 : 0;
}

#endif
