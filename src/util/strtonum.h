#ifndef UTIL_STRTONUM_H
#define UTIL_STRTONUM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "macros.h"

enum {
    HEX_INVALID = 0xF0,
};

// Decodes a single, hexadecimal digit and returns a numerical value
// between 0-15, or HEX_INVALID for invalid digits
static inline unsigned int hex_decode(unsigned char c)
{
    extern const uint8_t hex_table[256];
    return hex_table[c];
}

size_t size_str_width(size_t x) CONST_FN;
size_t buf_parse_uintmax(const char *str, size_t size, uintmax_t *valp) NONNULL_ARG(1);
size_t buf_parse_ulong(const char *str, size_t size, unsigned long *valp) NONNULL_ARG(1);
size_t buf_parse_uint(const char *str, size_t size, unsigned int *valp) NONNULL_ARG(1);
bool str_to_int(const char *str, int *valp) NONNULL_ARG(1);
bool str_to_uint(const char *str, unsigned int *valp) NONNULL_ARG(1);
bool str_to_size(const char *str, size_t *valp) NONNULL_ARG(1);
bool str_to_ulong(const char *str, unsigned long *valp) NONNULL_ARG(1);

#endif
