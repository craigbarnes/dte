#ifndef UTIL_ASCII_H
#define UTIL_ASCII_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "macros.h"

extern const uint8_t ascii_table[256];

typedef enum {
    ASCII_SPACE = 0x01,
    ASCII_DIGIT = 0x02,
    ASCII_CNTRL = 0x04,
    ASCII_REGEX = 0x08, // [$()*+.?[^{|\]
    ASCII_LOWER = 0x10,
    ASCII_UPPER = 0x20,
    ASCII_UNDERSCORE = 0x40,
    ASCII_NONASCII = 0x80,
    ASCII_ALPHA = ASCII_LOWER | ASCII_UPPER,
    ASCII_ALNUM = ASCII_ALPHA | ASCII_DIGIT,
    ASCII_WORDBYTE = ASCII_ALNUM | ASCII_UNDERSCORE | ASCII_NONASCII,
} AsciiCharType;

#define ascii_isspace(x) ascii_test(x, ASCII_SPACE)
#define ascii_iscntrl(x) ascii_test(x, ASCII_CNTRL)
#define ascii_islower(x) ascii_test(x, ASCII_LOWER)
#define ascii_isupper(x) ascii_test(x, ASCII_UPPER)
#define ascii_isalpha(x) ascii_test(x, ASCII_ALPHA)
#define ascii_isalnum(x) ascii_test(x, ASCII_ALNUM)
#define ascii_isprint(x) (!ascii_test(x, ASCII_CNTRL | ASCII_NONASCII))

#define is_alpha_or_underscore(x) ascii_test(x, ASCII_ALPHA | ASCII_UNDERSCORE)
#define is_alnum_or_underscore(x) ascii_test(x, ASCII_ALNUM | ASCII_UNDERSCORE)
#define is_regex_special_char(x) ascii_test(x, ASCII_REGEX)
#define is_word_byte(x) ascii_test(x, ASCII_WORDBYTE)

static inline bool ascii_test(unsigned char c, AsciiCharType mask)
{
    return (ascii_table[c] & mask) != 0;
}

static inline bool ascii_isblank(unsigned char c)
{
    return c == ' ' || c == '\t';
}

static inline bool ascii_isdigit(unsigned char c)
{
    return (unsigned int)c - '0' <= 9;
}

static inline bool ascii_is_digit_or_dot(unsigned char c)
{
    return ascii_isdigit(c) || c == '.';
}

static inline bool ascii_is_nonspace_cntrl(unsigned char c)
{
    return ascii_table[c] == ASCII_CNTRL;
}

static inline unsigned char ascii_tolower(unsigned char c)
{
    static_assert(ASCII_UPPER == 0x20);
    return c + (ascii_table[c] & ASCII_UPPER);
}

static inline unsigned char ascii_toupper(unsigned char c)
{
    static_assert(ASCII_LOWER << 1 == 0x20);
    return c - ((ascii_table[c] & ASCII_LOWER) << 1);
}

NONNULL_ARGS
static inline int ascii_strcmp_icase(const char *s1, const char *s2)
{
    unsigned char c1, c2;
    do {
        c1 = ascii_tolower(*s1++);
        c2 = ascii_tolower(*s2++);
    } while (c1 && c1 == c2);

    return c1 - c2;
}

NONNULL_ARGS
static inline bool ascii_streq_icase(const char *s1, const char *s2)
{
    return ascii_strcmp_icase(s1, s2) == 0;
}

static inline size_t ascii_blank_prefix_length(const char *str, size_t len)
{
    size_t i = 0;
    while (i < len && ascii_isblank(str[i])) {
        i++;
    }
    return i;
}

static inline bool strn_contains_char_type(const char *str, size_t len, AsciiCharType mask)
{
    for (size_t i = 0; i < len; i++) {
        if (ascii_test(str[i], mask)) {
            return true;
        }
    }
    return false;
}

#endif
