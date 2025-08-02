#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include "ascii.h"
#include "bit.h"
#include "macros.h"

static inline size_t fnv_1a_init(void)
{
    return (BITSIZE(size_t) >= 64) ? 14695981039346656037ULL : 2166136261U;
}

static inline size_t fnv_1a_prime(void)
{
    return (BITSIZE(size_t) >= 64) ? 1099511628211ULL : 16777619U;
}

// https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-31#name-fnv-basics
static inline size_t fnv_1a_hash(const char *str, size_t n)
{
    const size_t prime = fnv_1a_prime();
    size_t hash = fnv_1a_init();
    while (n--) {
        hash ^= (unsigned char)*str++;
        hash *= prime;
    }
    return hash;
}

static inline size_t fnv_1a_hash_icase(const char *str, size_t n)
{
    const size_t prime = fnv_1a_prime();
    size_t hash = fnv_1a_init();
    while (n--) {
        hash ^= ascii_tolower(*str++);
        hash *= prime;
    }
    return hash;
}

#endif
