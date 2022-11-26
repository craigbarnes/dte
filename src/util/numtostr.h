#ifndef UTIL_NUMTOSTR_H
#define UTIL_NUMTOSTR_H

#include <stddef.h>
#include <stdint.h>
#include "macros.h"

extern const char hex_encode_table[16];

// Encodes a byte of data as 2 hexadecimal digits
static inline char *hex_encode_byte(char *out, uint8_t byte)
{
    out[0] = hex_encode_table[byte >> 4];
    out[1] = hex_encode_table[byte & 0xF];
    return out;
}

// Encodes 24 bits from a uint32_t as 6 hexadecimal digits (fixed width)
static inline char *hex_encode_u24_fixed(char *out, uint32_t x)
{
    UNROLL_LOOP(6)
    for (size_t i = 0, n = 6; i < n; i++) {
        unsigned int shift = (n - i - 1) * 4;
        out[i] = hex_encode_table[(x >> shift) & 0xF];
    }
    return out;
}

size_t buf_umax_to_str(uintmax_t x, char *buf) NONNULL_ARGS;
size_t buf_uint_to_str(unsigned int x, char *buf) NONNULL_ARGS;

const char *umax_to_str(uintmax_t x) RETURNS_NONNULL;
const char *uint_to_str(unsigned int x) RETURNS_NONNULL;
const char *ulong_to_str(unsigned long x) RETURNS_NONNULL;

#endif
