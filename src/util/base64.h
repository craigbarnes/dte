#ifndef UTIL_BASE64_H
#define UTIL_BASE64_H

#include <stddef.h>
#include <stdint.h>
#include "macros.h"

extern const uint8_t base64_decode_table[256];
extern const char base64_encode_table[64];

enum {
    BASE64_PADDING = 1 << 6, // Return value for padding bytes (=)
    BASE64_INVALID = 1 << 7, // Return value for invalid bytes ([^A-Za-z0-9+/=])
};

// Decodes a single, base64 digit and returns a numerical value between 0-63,
// or one of the special enum values above
static inline unsigned int base64_decode(unsigned char c)
{
    return base64_decode_table[c];
}

// Like base64_decode(), but implemented in a way that's more amenable to
// constant propagation when `c` is known at compile-time
static inline unsigned int base64_decode_branchy(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return (c - 'a') + 26;
    }
    if (c >= '0' && c <= '9') {
        return (c - '0') + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    if (c == '=') {
        return BASE64_PADDING;
    }
    return BASE64_INVALID;
}

size_t base64_encode_block(const char *in, size_t ilen, char *out, size_t olen) NONNULL_ARGS;
void base64_encode_final(const char *in, size_t ilen, char out[4]) NONNULL_ARGS;

#endif
