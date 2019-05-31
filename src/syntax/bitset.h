#ifndef SYNTAX_BITSET_H
#define SYNTAX_BITSET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// This is a container type for storing a *set* of chars (bytes).
// It uses an array of 256 bits (32 bytes) for lookups, with each
// bit index used to determine whether or not the byte with that
// value is in the set.

typedef uint8_t BitSet[256 / 8];

static inline bool bitset_contains(const uint8_t *set, unsigned char ch)
{
    unsigned int byte = ch / 8;
    unsigned int bit = ch & 7;
    return set[byte] & 1u << bit;
}

static inline void bitset_invert(uint8_t *set)
{
    for (size_t i = 0; i < 32; i++) {
        set[i] = ~set[i];
    }
}

void bitset_add_pattern(uint8_t *set, const unsigned char *pattern);

#endif
