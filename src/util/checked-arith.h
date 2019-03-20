#ifndef UTIL_CHECKED_ARITH_H
#define UTIL_CHECKED_ARITH_H

#include <limits.h> // LONG_MAX
#include <stdbool.h>
#include <stddef.h> // size_t
#include <stdint.h> // SIZE_MAX
#include "macros.h"

#define HAS_GNUC5_OR_BUILTIN(b) (GNUC_AT_LEAST(5, 0) || HAS_BUILTIN(b))

static inline bool size_add_overflows(size_t a, size_t b, size_t *result)
{
#if HAS_GNUC5_OR_BUILTIN(__builtin_add_overflow)
    return __builtin_add_overflow(a, b, result);
#else
    if (unlikely(b > SIZE_MAX - a)) {
        return true;
    }
    *result = a + b;
    return false;
#endif
}

static inline bool size_multiply_overflows(size_t a, size_t b, size_t *result)
{
#if HAS_GNUC5_OR_BUILTIN(__builtin_mul_overflow)
    return __builtin_mul_overflow(a, b, result);
#else
    if (unlikely(a > 0 && b > SIZE_MAX / a)) {
        return true;
    }
    *result = a * b;
    return false;
#endif
}

static inline bool long_add_overflows(long a, long b, long *result)
{
#if HAS_GNUC5_OR_BUILTIN(__builtin_saddl_overflow)
    return __builtin_saddl_overflow(a, b, result);
#else
    if (unlikely(b > LONG_MAX - a)) {
        return true;
    }
    *result = a + b;
    return false;
#endif
}

static inline bool long_multiply_overflows(long a, long b, long *result)
{
#if HAS_GNUC5_OR_BUILTIN(__builtin_smull_overflow)
    return __builtin_smull_overflow(a, b, result);
#else
    if (unlikely(a > 0 && b > LONG_MAX / a)) {
        return true;
    }
    *result = a * b;
    return false;
#endif
}

#endif
