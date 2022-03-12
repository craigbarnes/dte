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

static inline size_t buf_parse_hex_uint(const char *str, size_t size, unsigned int *valp)
{
    unsigned int val = 0;
    size_t i;
    for (i = 0; i < size; i++) {
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

size_t size_str_width(size_t x) CONST_FN;
size_t buf_parse_uintmax(const char *str, size_t size, uintmax_t *valp) NONNULL_ARGS;
size_t buf_parse_ulong(const char *str, size_t size, unsigned long *valp) NONNULL_ARGS;
size_t buf_parse_uint(const char *str, size_t size, unsigned int *valp) NONNULL_ARGS;
size_t buf_parse_size(const char *str, size_t len, size_t *valp) NONNULL_ARGS;
bool str_to_int(const char *str, int *valp) NONNULL_ARGS;
bool str_to_uint(const char *str, unsigned int *valp) NONNULL_ARGS;
bool str_to_size(const char *str, size_t *valp) NONNULL_ARGS;
bool str_to_ulong(const char *str, unsigned long *valp) NONNULL_ARGS;
bool str_to_filepos(const char *str, size_t *line, size_t *col) NONNULL_ARGS;

#endif
