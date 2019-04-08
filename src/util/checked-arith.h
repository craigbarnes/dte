#ifndef UTIL_CHECKED_ARITH_H
#define UTIL_CHECKED_ARITH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "macros.h"

#define HAS_GNUC5_OR_BUILTIN(b) (GNUC_AT_LEAST(5, 0) || HAS_BUILTIN(b))

NONNULL_ARGS
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

NONNULL_ARGS
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

NONNULL_ARGS
static inline bool umax_add_overflows(uintmax_t a, uintmax_t b, uintmax_t *result)
{
#if HAS_GNUC5_OR_BUILTIN(__builtin_add_overflow)
    return __builtin_add_overflow(a, b, result);
#else
    if (unlikely(b > UINTMAX_MAX - a)) {
        return true;
    }
    *result = a + b;
    return false;
#endif
}

NONNULL_ARGS
static inline bool umax_multiply_overflows(uintmax_t a, uintmax_t b, uintmax_t *result)
{
#if HAS_GNUC5_OR_BUILTIN(__builtin_mul_overflow)
    return __builtin_mul_overflow(a, b, result);
#else
    if (unlikely(a > 0 && b > UINTMAX_MAX / a)) {
        return true;
    }
    *result = a * b;
    return false;
#endif
}

#endif
