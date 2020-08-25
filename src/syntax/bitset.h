#ifndef SYNTAX_BITSET_H
#define SYNTAX_BITSET_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"

#define BITSET_WORD_BITS (sizeof(BitSetWord) * CHAR_BIT)
#define BITSET_BIT_MASK (BITSET_WORD_BITS - 1)
#define BITSET_NR_WORDS(bits) (((bits) + BITSET_WORD_BITS - 1) / BITSET_WORD_BITS)
#define BITSET_INVERT(set) bitset_invert(set, ARRAY_COUNT(set))

typedef unsigned long BitSetWord;

static inline bool bitset_contains(const BitSetWord *set, unsigned char ch)
{
    unsigned int word = ch / BITSET_WORD_BITS;
    unsigned int bit = ch & BITSET_BIT_MASK;
    return set[word] & ((BitSetWord)1) << bit;
}

static inline void bitset_add(BitSetWord *set, unsigned char ch)
{
    unsigned int word = ch / BITSET_WORD_BITS;
    unsigned int bit = ch & BITSET_BIT_MASK;
    set[word] |= ((BitSetWord)1) << bit;
}

static inline void bitset_add_char_range(BitSetWord *set, const unsigned char *r)
{
    for (size_t i = 0; r[i]; i++) {
        unsigned int ch = r[i];
        bitset_add(set, ch);
        if (r[i + 1] == '-' && r[i + 2]) {
            // Add char range
            for (ch = ch + 1; ch <= r[i + 2]; ch++) {
                bitset_add(set, ch);
            }
            i += 2;
        }
    }
}

static inline void bitset_invert(BitSetWord *set, size_t nr_words)
{
    for (size_t i = 0; i < nr_words; i++) {
        set[i] = ~set[i];
    }
}

#endif
