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

#if GNUC_AT_LEAST(5, 0) || HAS_BUILTIN(__builtin_mul_overflow)
    #define CHECKED_ADD(a, b, res) return __builtin_add_overflow(a, b, res)
    #define CHECKED_MUL(a, b, res) return __builtin_mul_overflow(a, b, res)
#elif __STDC_VERSION__ >= 202311L
    #include <stdckdint.h>
    #define CHECKED_ADD(a, b, res) return ckd_add(res, a, b)
    #define CHECKED_MUL(a, b, res) return ckd_mul(res, a, b)
#else
    #define CHECKED_ADD(a, b, res) \
        if (unlikely(b > UNSIGNED_MAX_VALUE(a) - a)) return true; else {*res = a + b; return false;}
    #define CHECKED_MUL(a, b, res) \
        if (unlikely(a > 0 && b > UNSIGNED_MAX_VALUE(a) / a)) return true; else {*res = a * b; return false;}
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
static inline size_t wrapping_increment(size_t x, size_t modulus)
{
    BUG_ON(modulus == 0);
    BUG_ON(x >= modulus);
    return (x + 1 < modulus) ? x + 1 : 0;
}

// As above, but for decrementing `x` instead of incrementing it
static inline size_t wrapping_decrement(size_t x, size_t modulus)
{
    BUG_ON(modulus == 0);
    BUG_ON(x >= modulus);
    return (x ? x : modulus) - 1;
}

static inline size_t saturating_increment(size_t x, size_t max)
{
    BUG_ON(x > max);
    return x + (x < max);
}

static inline size_t saturating_decrement(size_t x)
{
    return x - (x > 0);
}

static inline size_t saturating_subtract(size_t a, size_t b)
{
    return a - MIN(a, b);
}

size_t xmul_(size_t a, size_t b);
size_t xadd(size_t a, size_t b);

// Equivalent to `a * b`, but calling fatal_error() for arithmetic overflow.
// Note that if either operand is 2, adding 1 to the result can never
// overflow (often useful when doubling a buffer plus 1 extra byte). Similar
// observations can be made when multiplying by other powers of 2 (except 1)
// due to the equivalence with left shifting.
static inline size_t xmul(size_t a, size_t b)
{
    // If either argument is known at compile-time to be 1, the multiplication
    // can't overflow and is thus safe to be inlined without checks
    if ((IS_CT_CONSTANT(a) && a <= 1) || (IS_CT_CONSTANT(b) && b <= 1)) {
        return a * b; // GCOVR_EXCL_LINE
    }
    // Otherwise, emit a call to the checked implementation
    return xmul_(a, b);
}

static inline size_t xadd3(size_t a, size_t b, size_t c)
{
    return xadd(a, xadd(b, c));
}

#endif
