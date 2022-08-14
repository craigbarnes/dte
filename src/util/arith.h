#ifndef UTIL_ARITH_H
#define UTIL_ARITH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "debug.h"
#include "macros.h"

// Calculate the maximum representable value for __typeof__(x), where x
// is an unsigned integer and sizeof(x) >= sizeof(int)
#define UNSIGNED_MAX_VALUE(x) (~((x) ^ (x)))

#if HAS_BUILTIN(__builtin_mul_overflow) || GNUC_AT_LEAST(5, 0)
    #define CHECKED_MUL(a, b, res) return __builtin_mul_overflow(a, b, res)
#else
    #define CHECKED_MUL(a, b, res) \
        if (unlikely(a > 0 && b > UNSIGNED_MAX_VALUE(a) / a)) return true; else {*res = a * b; return false;}
#endif

#if HAS_BUILTIN(__builtin_add_overflow) || GNUC_AT_LEAST(5, 0)
    #define CHECKED_ADD(a, b, res) return __builtin_add_overflow(a, b, res)
#else
    #define CHECKED_ADD(a, b, res) \
        if (unlikely(b > UNSIGNED_MAX_VALUE(a) - a)) return true; else {*res = a + b; return false;}
#endif

static inline bool umax_multiply_overflows(uintmax_t a, uintmax_t b, uintmax_t *result)
{
    CHECKED_MUL(a, b, result);
}

static inline bool size_multiply_overflows(size_t a, size_t b, size_t *result)
{
    CHECKED_MUL(a, b, result);
}

static inline bool umax_add_overflows(uintmax_t a, uintmax_t b, uintmax_t *result)
{
    CHECKED_ADD(a, b, result);
}

static inline bool size_add_overflows(size_t a, size_t b, size_t *result)
{
    CHECKED_ADD(a, b, result);
}

// This is equivalent to `(x + 1) % modulus`, given the constraints
// imposed by BUG_ON(), but avoids expensive divisions by a non-constant
// and/or non-power-of-2
static inline size_t size_increment_wrapped(size_t x, size_t modulus)
{
    BUG_ON(modulus == 0);
    BUG_ON(x >= modulus);
    return (x + 1 < modulus) ? x + 1 : 0;
}

// As above, but for decrementing `x` instead of incrementing it
static inline size_t size_decrement_wrapped(size_t x, size_t modulus)
{
    BUG_ON(modulus == 0);
    BUG_ON(x >= modulus);
    return (x ? x : modulus) - 1;
}

#endif
