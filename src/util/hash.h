#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#include <stddef.h>
#include <stdint.h>
#include "ascii.h"

#define FNV_32_INIT UINT32_C(2166136261)
#define FNV_64_INIT UINT64_C(14695981039346656037)
#define FNV_32_PRIME UINT32_C(16777619)
#define FNV_64_PRIME UINT64_C(1099511628211)

static inline uint32_t fnv_1a_32_hash(const char *str, size_t len)
{
    uint32_t hash = FNV_32_INIT;
    while (len--) {
        uint32_t c = *str++;
        hash ^= c;
        hash *= FNV_32_PRIME;
    }
    return hash;
}

static inline uint32_t fnv_1a_32_hash_icase(const char *str, size_t len)
{
    uint32_t hash = FNV_32_INIT;
    while (len--) {
        uint32_t c = ascii_tolower(*str++);
        hash ^= c;
        hash *= FNV_32_PRIME;
    }
    return hash;
}

static inline uint64_t fnv_1a_64_hash(const char *str, size_t len)
{
    uint64_t hash = FNV_64_INIT;
    while (len--) {
        uint64_t c = *str++;
        hash ^= c;
        hash *= FNV_64_PRIME;
    }
    return hash;
}

static inline uint64_t fnv_1a_64_hash_icase(const char *str, size_t len)
{
    uint64_t hash = FNV_64_INIT;
    while (len--) {
        uint64_t c = ascii_tolower(*str++);
        hash ^= c;
        hash *= FNV_64_PRIME;
    }
    return hash;
}

#endif
