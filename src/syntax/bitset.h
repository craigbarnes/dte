#ifndef SYNTAX_BITSET_H
#define SYNTAX_BITSET_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include "util/debug.h"
#include "util/macros.h"

#define BITSET_WORD_BITS BITSIZE(BitSetWord)
#define BITSET_BIT_MASK (BITSET_WORD_BITS - 1)
#define BITSET_NR_WORDS(bits) (((bits) + BITSET_WORD_BITS - 1) / BITSET_WORD_BITS)
#define BITSET_INVERT(set) bitset_invert(set, ARRAYLEN(set))

typedef unsigned long BitSetWord;

static inline size_t bitset_word_idx(unsigned char ch)
{
    return ch / BITSET_WORD_BITS;
}

static inline unsigned int bitset_bit_idx(unsigned char ch)
{
    return ch & BITSET_BIT_MASK;
}

static inline BitSetWord bitset_bit(unsigned char ch)
{
    return ((BitSetWord)1) << bitset_bit_idx(ch);
}

static inline bool bitset_contains(const BitSetWord *set, unsigned char ch)
{
    size_t idx = bitset_word_idx(ch);
    return set[idx] & bitset_bit(ch);
}

static inline void bitset_add(BitSetWord *set, unsigned char ch)
{
    size_t idx = bitset_word_idx(ch);
    set[idx] |= bitset_bit(ch);
}

static inline BitSetWord bitset_word_max(void)
{
    BitSetWord m = 0;
    return ~m;
}

// Return a word in which the bit for `ch` and all higher bits are set
static inline BitSetWord bitset_start_mask(unsigned char ch)
{
    return bitset_word_max() << bitset_bit_idx(ch);
}

// Return a word in which the bit for `ch` and all lower bits are set
static inline BitSetWord bitset_end_mask(unsigned char ch)
{
    BitSetWord bit = bitset_bit(ch);
    BUG_ON(bit == 0);
    return bit | (bit - 1);
}

static inline void bitset_add_char_range(BitSetWord *set, const char *r)
{
    for (size_t i = 0; r[i]; i++) {
        unsigned int first_char = (unsigned char)r[i];
        if (r[i + 1] != '-' || r[i + 2] == '\0') {
            // Not a range; set bit for a single character
            bitset_add(set, first_char);
            continue;
        }

        i += 2;
        unsigned int last_char = (unsigned char)r[i];
        if (unlikely(first_char > last_char)) {
            continue;
        }

        size_t first_word = bitset_word_idx(first_char);
        size_t last_word = bitset_word_idx(last_char);
        BitSetWord start_mask = bitset_start_mask(first_char);
        BitSetWord end_mask = bitset_end_mask(last_char);

        if (first_word == last_word) {
            // Only 1 word affected; use the intersection of both masks
            set[first_word] |= start_mask & end_mask;
            continue;
        }

        // Set partial ranges in first/last word, then saturate any others
        set[first_word] |= start_mask;
        set[last_word] |= end_mask;
        for (size_t w = first_word + 1; w < last_word; w++) {
            set[w] = bitset_word_max();
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
