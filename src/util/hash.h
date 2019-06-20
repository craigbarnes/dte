#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#include <stddef.h>
#include <stdint.h>
#include "ascii.h"

#define FNV_BASE UINT32_C(2166136261)
#define FNV_PRIME UINT32_C(16777619)

static inline uint32_t fnv_1a_hash(const char *str, size_t len)
{
    uint32_t hash = FNV_BASE;
    while (len--) {
        uint32_t c = *str++;
        hash ^= c;
        hash *= FNV_PRIME;
    }
    return hash;
}

static inline uint32_t fnv_1a_hash_icase(const char *str, size_t len)
{
    uint32_t hash = FNV_BASE;
    while (len--) {
        uint32_t c = ascii_tolower(*str++);
        hash ^= c;
        hash *= FNV_PRIME;
    }
    return hash;
}

#endif
