#ifndef SYNTAX_BITSET_H
#define SYNTAX_BITSET_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// This is a container type for storing a *set* of chars (bytes).
// It uses an array of 256 bits (32 bytes) for lookups, with each
// bit index used to determine whether or not the byte with that
// value is in the set.

#define BITSET_WORD_BITS (sizeof(BitSetWord) * CHAR_BIT)
#define BITSET_BIT_MASK (BITSET_WORD_BITS - 1)
#define BITSET_NR_WORDS (256 / BITSET_WORD_BITS)

typedef uint32_t BitSetWord;
typedef BitSetWord BitSet[BITSET_NR_WORDS];

static inline bool bitset_contains(const BitSetWord *set, unsigned char ch)
{
    unsigned int word = ch / BITSET_WORD_BITS;
    unsigned int bit = ch & BITSET_BIT_MASK;
    return set[word] & ((BitSetWord)1) << bit;
}

static inline void bitset_invert(BitSetWord *set)
{
    for (size_t i = 0; i < BITSET_NR_WORDS; i++) {
        set[i] = ~set[i];
    }
}

void bitset_add_pattern(BitSetWord *set, const unsigned char *pattern);

#endif
