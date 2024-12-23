#ifndef UTIL_STRTONUM_H
#define UTIL_STRTONUM_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "macros.h"

extern const uint8_t hex_decode_table[256];

enum {
    HEX_INVALID = 0xF0,
};

// Decodes a single, hexadecimal digit and returns a numerical value
// between 0-15, or HEX_INVALID for invalid digits
static inline unsigned int hex_decode(unsigned char c)
{
    return hex_decode_table[c];
}

static inline bool ascii_isxdigit(unsigned char c)
{
    return hex_decode(c) <= 0xF;
}

static inline size_t ascii_hex_prefix_length(const char *str, size_t len)
{
    size_t i = 0;
    while (i < len && ascii_isxdigit(str[i])) {
        i++;
    }
    return i;
}

WARN_UNUSED_RESULT
static inline size_t buf_parse_hex_uint(const char *str, size_t len, unsigned int *valp)
{
    unsigned int val = 0;
    size_t i;
    for (i = 0; i < len; i++) {
        unsigned int x = hex_decode(str[i]);
        if (unlikely(x > 0xF)) {
            break;
        }
        if (unlikely(val > (UINT_MAX >> 4))) {
            return 0; // Overflow
        }
        val = val << 4 | x;
    }
    *valp = val;
    return i;
}

size_t size_str_width(size_t x) CONST_FN WARN_UNUSED_RESULT;
size_t buf_parse_uintmax(const char *str, size_t len, uintmax_t *valp) WARN_UNUSED_RESULT;
size_t buf_parse_ulong(const char *str, size_t len, unsigned long *valp) WARN_UNUSED_RESULT;
size_t buf_parse_uint(const char *str, size_t len, unsigned int *valp) WARN_UNUSED_RESULT;
size_t buf_parse_size(const char *str, size_t len, size_t *valp) WARN_UNUSED_RESULT;
bool str_to_int(const char *str, int *valp) NONNULL_ARGS WARN_UNUSED_RESULT;
bool str_to_uint(const char *str, unsigned int *valp) NONNULL_ARGS WARN_UNUSED_RESULT;
bool str_to_size(const char *str, size_t *valp) NONNULL_ARGS WARN_UNUSED_RESULT;
bool str_to_ulong(const char *str, unsigned long *valp) NONNULL_ARGS WARN_UNUSED_RESULT;
bool str_to_filepos(const char *str, size_t *linep, size_t *colp) NONNULL_ARGS WARN_UNUSED_RESULT;
bool str_to_xfilepos(const char *str, size_t *linep, size_t *colp) NONNULL_ARGS WARN_UNUSED_RESULT;
intmax_t parse_filesize(const char *str) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
