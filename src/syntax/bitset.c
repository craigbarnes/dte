#include "bitset.h"
#include "../util/macros.h"

static void bitset_add(BitSetWord *set, unsigned char ch)
{
    static_assert(256 % BITSET_WORD_BITS == 0);
    unsigned int word = ch / BITSET_WORD_BITS;
    unsigned int bit = ch & BITSET_BIT_MASK;
    set[word] |= ((BitSetWord)1) << bit;
}

void bitset_add_pattern(BitSetWord *set, const unsigned char *pattern)
{
    for (size_t i = 0; pattern[i]; i++) {
        unsigned int ch = pattern[i];
        bitset_add(set, ch);
        if (pattern[i + 1] == '-' && pattern[i + 2]) {
            // Add char range
            for (ch = ch + 1; ch <= pattern[i + 2]; ch++) {
                bitset_add(set, ch);
            }
            i += 2;
        }
    }
}
