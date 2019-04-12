#ifndef UTIL_ASCII_H
#define UTIL_ASCII_H

#include <stdbool.h>
#include <stdint.h>
#include "macros.h"

extern const uint8_t ascii_table[256];

#define ASCII_SPACE 0x01
#define ASCII_DIGIT 0x02
#define ASCII_CNTRL 0x04
// 0x08
#define ASCII_LOWER 0x10
#define ASCII_UPPER 0x20
#define ASCII_UNDERSCORE 0x40
#define ASCII_NONASCII 0x80

#define ASCII_ALPHA (ASCII_LOWER | ASCII_UPPER)
#define ASCII_ALNUM (ASCII_ALPHA | ASCII_DIGIT)
#define ASCII_WORDBYTE (ASCII_ALNUM | ASCII_UNDERSCORE | ASCII_NONASCII)

#define ascii_test(x, mask) ((ascii_table[(unsigned char)(x)] & (mask)) != 0)
#define ascii_isspace(x) ascii_test(x, ASCII_SPACE)
#define ascii_isdigit(x) ascii_test(x, ASCII_DIGIT)
#define ascii_iscntrl(x) ascii_test(x, ASCII_CNTRL)
#define ascii_islower(x) ascii_test(x, ASCII_LOWER)
#define ascii_isupper(x) ascii_test(x, ASCII_UPPER)
#define ascii_isalpha(x) ascii_test(x, ASCII_ALPHA)
#define ascii_isalnum(x) ascii_test(x, ASCII_ALNUM)
#define ascii_isprint(x) (!ascii_test(x, ASCII_CNTRL | ASCII_NONASCII))
#define ascii_is_nonspace_cntrl(x) (ascii_table[(unsigned char)(x)] == ASCII_CNTRL)

#define is_alpha_or_underscore(x) ascii_test(x, ASCII_ALPHA | ASCII_UNDERSCORE)
#define is_alnum_or_underscore(x) ascii_test(x, ASCII_ALNUM | ASCII_UNDERSCORE)
#define is_word_byte(x) ascii_test(x, ASCII_WORDBYTE)

CONST_FN
static inline bool ascii_isblank(unsigned char c)
{
    return c == ' ' || c == '\t';
}

CONST_FN
static inline unsigned char ascii_tolower(unsigned char c)
{
    return c + (ascii_table[c] & ASCII_UPPER);
}

CONST_FN
static inline unsigned char ascii_toupper(unsigned char c)
{
    return c - ((ascii_table[c] & ASCII_LOWER) << 1);
}

int hex_decode(int ch) CONST_FN;

#endif
