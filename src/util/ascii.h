#ifndef UTIL_ASCII_H
#define UTIL_ASCII_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "macros.h"

extern const uint8_t ascii_table[256];
extern const int8_t hex_table[256];

#define ASCII_SPACE 0x01
#define ASCII_DIGIT 0x02
#define ASCII_CNTRL 0x04
#define ASCII_REGEX 0x08
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
#define ascii_isxdigit(x) (hex_decode(x) != -1)

#define is_alpha_or_underscore(x) ascii_test(x, ASCII_ALPHA | ASCII_UNDERSCORE)
#define is_alnum_or_underscore(x) ascii_test(x, ASCII_ALNUM | ASCII_UNDERSCORE)
#define is_regex_special_char(x) ascii_test(x, ASCII_REGEX)
#define is_word_byte(x) ascii_test(x, ASCII_WORDBYTE)

static inline bool ascii_isblank(unsigned char c)
{
    return c == ' ' || c == '\t';
}

static inline bool ascii_is_nonspace_cntrl(unsigned char c)
{
    return ascii_table[c] == ASCII_CNTRL;
}

static inline unsigned char ascii_tolower(unsigned char c)
{
    return c + (ascii_table[c] & ASCII_UPPER);
}

static inline unsigned char ascii_toupper(unsigned char c)
{
    return c - ((ascii_table[c] & ASCII_LOWER) << 1);
}

NONNULL_ARGS
static inline bool ascii_streq_icase(const char *s1, const char *s2)
{
    unsigned char c1, c2;
    bool chars_equal;
    size_t i = 0;

    // Iterate to the index where the strings differ or a NUL byte is found
    do {
        c1 = ascii_tolower(s1[i]);
        c2 = ascii_tolower(s2[i]);
        chars_equal = (c1 == c2);
        i++;
    } while (c1 && chars_equal);

    // If the loop terminated because a NUL byte was found and the
    // last characters were the same, both strings terminate in the
    // same place and are therefore equal
    return chars_equal;
}

NONNULL_ARGS
static inline bool mem_equal_icase(const void *p1, const void *p2, size_t n)
{
    const unsigned char *s1 = p1;
    const unsigned char *s2 = p2;
    while (n) {
        if (ascii_tolower(*s1++) != ascii_tolower(*s2++)) {
            return false;
        }
        n--;
    }
    return true;
}

static inline int hex_decode(unsigned char c)
{
    return hex_table[c];
}

#endif
