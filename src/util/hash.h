#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "ascii.h"
#include "macros.h"

#define FNV_32_INIT UINT32_C(2166136261)
#define FNV_64_INIT UINT64_C(14695981039346656037)
#define FNV_32_PRIME UINT32_C(16777619)
#define FNV_64_PRIME UINT64_C(1099511628211)

static inline size_t fnv_1a_init(void)
{
    static_assert(8 * CHAR_BIT == 64);
    return (sizeof(size_t) >= 8) ? FNV_64_INIT : FNV_32_INIT;
}

static inline size_t fnv_1a_prime(void)
{
    return (sizeof(size_t) >= 8) ? FNV_64_PRIME : FNV_32_PRIME;
}

static inline size_t fnv_1a_hash(const unsigned char *str, size_t n)
{
    const size_t prime = fnv_1a_prime();
    size_t hash = fnv_1a_init();
    while (n--) {
        hash ^= *str++;
        hash *= prime;
    }
    return hash;
}

static inline size_t fnv_1a_hash_icase(const unsigned char *str, size_t n)
{
    const size_t prime = fnv_1a_prime();
    size_t hash = fnv_1a_init();
    while (n--) {
        hash ^= ascii_tolower(*str++);
        hash *= prime;
    }
    return hash;
}

// NOTE: returns 0 if x is greater than the largest size_t power of 2
static inline size_t round_size_to_next_power_of_2(size_t x)
{
    if (unlikely(x == 0)) {
        return 1;
    }
    x--;
    UNROLL_LOOP(8)
    for (size_t i = 1, n = sizeof(size_t) * CHAR_BIT; i < n; i <<= 1) {
        x |= x >> i;
    }
    return x + 1;
}

#endif
