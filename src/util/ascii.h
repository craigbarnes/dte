#ifndef UTIL_ASCII_H
#define UTIL_ASCII_H

#include <stdbool.h>
#include "macros.h"

extern const unsigned char ascii_table[256];

#define ASCII_SPACE 0x01
#define ASCII_DIGIT 0x02
#define ASCII_LOWER 0x04
#define ASCII_UPPER 0x08

#define ascii_test(x, mask) ((ascii_table[(unsigned char)(x)] & (mask)) != 0)
#define ascii_isspace(x) ascii_test(x, ASCII_SPACE)
#define ascii_isdigit(x) ascii_test(x, ASCII_DIGIT)
#define ascii_islower(x) ascii_test(x, ASCII_LOWER)
#define ascii_isupper(x) ascii_test(x, ASCII_UPPER)
#define ascii_isalpha(x) ascii_test(x, ASCII_LOWER | ASCII_UPPER)
#define ascii_isalnum(x) ascii_test(x, ASCII_LOWER | ASCII_UPPER | ASCII_DIGIT)

static inline int PURE ascii_tolower(int x)
{
    if (ascii_isupper(x)) {
        return x | 0x20;
    }
    return x;
}

static inline int PURE ascii_toupper(int x)
{
    if (ascii_islower(x)) {
        return x & ~0x20;
    }
    return x;
}

static inline bool PURE is_word_byte(unsigned char byte)
{
    return ascii_isalnum(byte) || byte == '_' || byte > 0x7f;
}

int hex_decode(int ch) CONST_FN;

#endif
