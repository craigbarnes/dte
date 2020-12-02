#ifndef UTIL_BASE64_H
#define UTIL_BASE64_H

#include <stdint.h>

enum {
    BASE64_PADDING = 1 << 6, // Return value for padding bytes (=)
    BASE64_INVALID = 1 << 7, // Return value for invalid bytes ([^A-Za-z0-9+/=])
};

// Decodes a single, base64 digit and returns a numerical value between 0-63,
// or one of the special enum values above.
static inline unsigned int base64_decode(unsigned char c)
{
    extern const uint8_t base64_table[256];
    return base64_table[c];
}

#endif
