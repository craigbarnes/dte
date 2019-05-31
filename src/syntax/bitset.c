#include "bitset.h"

static void bitset_add(uint8_t *set, unsigned char ch)
{
    unsigned int byte = ch / 8;
    unsigned int bit = ch & 7;
    set[byte] |= 1u << bit;
}

void bitset_add_pattern(uint8_t *set, const unsigned char *pattern)
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
