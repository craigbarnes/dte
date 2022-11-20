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

size_t base64_encode_block(const char *in, size_t ilen, char *out, size_t olen) NONNULL_ARGS;
void base64_encode_final(const char *in, size_t ilen, char out[4]) NONNULL_ARGS;

#endif
