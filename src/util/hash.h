#ifndef UTIL_HASH_H
#define UTIL_HASH_H

#include <stddef.h>
#include <stdint.h>
#include "ascii.h"

#define FNV_32_INIT UINT32_C(2166136261)
#define FNV_64_INIT UINT64_C(14695981039346656037)
#define FNV_32_PRIME UINT32_C(16777619)
#define FNV_64_PRIME UINT64_C(1099511628211)

static inline uint32_t fnv_1a_32_hash(const unsigned char *str, size_t n)
{
    uint32_t hash = FNV_32_INIT;
    while (n--) {
        hash ^= *str++;
        hash *= FNV_32_PRIME;
    }
    return hash;
}

static inline uint32_t fnv_1a_32_hash_icase(const unsigned char *str, size_t n)
{
    uint32_t hash = FNV_32_INIT;
    while (n--) {
        hash ^= ascii_tolower(*str++);
        hash *= FNV_32_PRIME;
    }
    return hash;
}

static inline uint64_t fnv_1a_64_hash(const unsigned char *str, size_t n)
{
    uint64_t hash = FNV_64_INIT;
    while (n--) {
        hash ^= *str++;
        hash *= FNV_64_PRIME;
    }
    return hash;
}

static inline uint64_t fnv_1a_64_hash_icase(const unsigned char *str, size_t n)
{
    uint64_t hash = FNV_64_INIT;
    while (n--) {
        hash ^= ascii_tolower(*str++);
        hash *= FNV_64_PRIME;
    }
    return hash;
}

#endif
