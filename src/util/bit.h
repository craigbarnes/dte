#ifndef UTIL_BIT_H
#define UTIL_BIT_H

#include <stdint.h>
#include "debug.h"
#include "macros.h"

/*
 * The default C compiler on FreeBSD 14.1 (Clang 18.1.5) accepts the
 * -std=gnu23 command-line option and then defines __STDC_VERSION__ as
 * 202311L, while not actually providing a C23 conforming libc (the
 * <stdbit.h> header is missing). This guard condition originally used
 * `||` instead of `&&`, but it was changed so as to work around this
 * disregard for standards. Fortunately this doesn't really have any
 * downsides for other platforms, since __has_include() is also part
 * of C23.
 */
#if __STDC_VERSION__ >= 202311L && HAS_INCLUDE(<stdbit.h>)
    #include <stdbit.h>
    #define USE_STDBIT(fn, arg) return fn(arg)
#else
    #define USE_STDBIT(fn, arg)
#endif

#define U64(x) (UINT64_C(x))
#define U32(x) (UINT32_C(x))

#ifdef HAS_BUILTIN_TYPES_COMPATIBLE_P
    #define HAS_COMPATIBLE_BUILTIN(arg, type, fn) \
        (GNUC_AT_LEAST(3, 4) || HAS_BUILTIN(__builtin_ ## fn)) \
        && __builtin_types_compatible_p(__typeof__(arg), type)

    // If there's an appropriate built-in function for `arg`, emit
    // a call to it (and eliminate everything below it as dead code)
    #define USE_BUILTIN(fn, arg) \
        if (HAS_COMPATIBLE_BUILTIN(arg, unsigned long long, fn ## ll)) { \
            return __builtin_ ## fn ## ll(arg); \
        } else if (HAS_COMPATIBLE_BUILTIN(arg, unsigned long, fn ## l)) { \
            return __builtin_ ## fn ## l(arg); \
        } else if (HAS_COMPATIBLE_BUILTIN(arg, unsigned int, fn)) { \
            return __builtin_ ## fn(arg); \
        }
#else
    #define USE_BUILTIN(fn, arg)
#endif

// Population count (cardinality) of set bits
static inline unsigned int u64_popcount(uint64_t n)
{
    USE_STDBIT(stdc_count_ones, n);
    USE_BUILTIN(popcount, n);
    n -= ((n >> 1) & U64(0x5555555555555555));
    n = (n & U64(0x3333333333333333)) + ((n >> 2) & U64(0x3333333333333333));
    n = (n + (n >> 4)) & U64(0x0F0F0F0F0F0F0F0F);
    return (n * U64(0x0101010101010101)) >> 56;
}

static inline unsigned int u32_popcount(uint32_t n)
{
    USE_STDBIT(stdc_count_ones, n);
    USE_BUILTIN(popcount, n);
    n -= ((n >> 1) & U32(0x55555555));
    n = (n & U32(0x33333333)) + ((n >> 2) & U32(0x33333333));
    n = (n + (n >> 4)) & U32(0x0F0F0F0F);
    return (n * U32(0x01010101)) >> 24;
}

// Count trailing zeros
static inline unsigned int u32_ctz(uint32_t n)
{
    BUG_ON(n == 0);
    USE_STDBIT(stdc_trailing_zeros, n);
    USE_BUILTIN(ctz, n);
    return u32_popcount(~n & (n - 1));
}

// Find position of first (least significant) set bit (starting at 1)
static inline unsigned int u32_ffs(uint32_t n)
{
    USE_BUILTIN(ffs, n);
    return n ? u32_ctz(n) + 1 : 0;
}

// Extract (isolate) least significant set bit
static inline uint32_t u32_lsbit(uint32_t x)
{
    return x ? 1u << (u32_ffs(x) - 1) : 0;
}

// Count leading zeros
static inline unsigned int u64_clz(uint64_t x)
{
    BUG_ON(x == 0);
    USE_STDBIT(stdc_leading_zeros, x);
    USE_BUILTIN(clz, x);
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return u64_popcount(~x);
}

// Calculate the number of significant bits in `x` (the minimum number
// of base 2 digits needed to represent it). Note that this returns 0
// for x=0, but see umax_count_base16_digits() (below) for an example
// of how that can be special cased.
static inline unsigned int umax_bitwidth(uintmax_t x)
{
    USE_STDBIT(stdc_bit_width, x);

#if HAS_BUILTIN(__builtin_stdc_bit_width)
    return __builtin_stdc_bit_width(x);
#endif

    if (BITSIZE(x) == 64) {
        return x ? 64u - u64_clz(x) : 0;
    }

    unsigned int width = 0;
    while (x) {
        x >>= 1;
        width++;
    }
    return width;
}

// Calculate the number of hexadecimal digits (0-F) needed to represent
// `x` as a string (which is 1 in the case of x=0)
static inline size_t umax_count_base16_digits(uintmax_t x)
{
    unsigned int bit_width = umax_bitwidth(x);

    // `bit_width` can only be 0 when `x` is 0, so this simply ensures
    // 1 is returned in that case
    BUG_ON(!bit_width != !x); // Improves code gen
    unsigned int base2_digits = bit_width + !x;

    // This is equivalent to `(base2_digits >> 2) + (base2_digits & 3)`,
    // but GCC seems to generate slightly better code when done this way
    return next_multiple(base2_digits, 4) / 4;
}

#endif
