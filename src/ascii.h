#ifndef ASCII_H
#define ASCII_H

#include <stdbool.h>
#include "macros.h"

extern const unsigned char dte_ascii_table[256];

#define ASCII_SPACE 0x01
#define ASCII_DIGIT 0x02
#define ASCII_LOWER 0x04
#define ASCII_UPPER 0x08
#define ASCII_GLOB_SPECIAL 0x10
#define ASCII_REGEX_SPECIAL 0x20
#define ASCII_HEX_LOWER 0x40
#define ASCII_HEX_UPPER 0x80

#define sane_istest(x,mask) ((dte_ascii_table[(unsigned char)(x)] & (mask)) != 0)
#define ascii_isspace(x) sane_istest(x, ASCII_SPACE)
#define ascii_isdigit(x) sane_istest(x, ASCII_DIGIT)
#define ascii_islower(x) sane_istest(x, ASCII_LOWER)
#define ascii_isupper(x) sane_istest(x, ASCII_UPPER)
#define ascii_isalpha(x) sane_istest(x, ASCII_LOWER | ASCII_UPPER)
#define ascii_isalnum(x) sane_istest(x, ASCII_LOWER | ASCII_UPPER | ASCII_DIGIT)
#define ascii_isxdigit(x) sane_istext(x, ASCII_DIGIT | ASCII_HEX_LOWER | ASCII_HEX_UPPER)
#define ascii_is_glob_special(x) sane_istest(x, ASCII_GLOB_SPECIAL)
#define ascii_is_regex_special(x) sane_istest(x, ASCII_GLOB_SPECIAL | ASCII_REGEX_SPECIAL)

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

int hex_decode(int ch) PURE;

#endif
