#ifndef UTIL_BASE64_H
#define UTIL_BASE64_H

#include <stdint.h>

extern const uint8_t base64_table[256];

static inline unsigned int base64_decode(unsigned char c)
{
    return base64_table[c];
}

#endif
