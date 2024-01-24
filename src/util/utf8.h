#ifndef UTIL_UTF8_H
#define UTIL_UTF8_H

#include <stddef.h>
#include "macros.h"
#include "unicode.h"

enum {
    // Longest UTF-8 sequence (in bytes) permitted by RFC 3629
    UTF8_MAX_SEQ_LEN = 4,
};

static inline size_t u_char_size(CodePoint u)
{
    // If `u` is invalid, set `adj` to 3 and use to adjust the calculation
    // so that 1 is returned. This indicates an invalid byte in the UTF-8
    // byte sequence.
    size_t inv = (u > UNICODE_MAX_VALID_CODEPOINT);
    size_t adj = inv | (inv << 1);

    return 1 + (u > 0x7F) + (u > 0x7FF) + (u > 0xFFFF) - adj;
}

size_t u_str_width(const unsigned char *str);

CodePoint u_prev_char(const unsigned char *str, size_t *idx);
CodePoint u_str_get_char(const unsigned char *str, size_t *idx);
CodePoint u_get_char(const unsigned char *str, size_t size, size_t *idx);
CodePoint u_get_nonascii(const unsigned char *str, size_t size, size_t *idx);

size_t u_set_char_raw(char *buf, CodePoint u);
size_t u_set_char(char *buf, CodePoint u);
size_t u_set_hex(char *buf, CodePoint u);

/*
 * Total width of skipped characters is stored back to @width.
 *
 * Stored @width can be 1 more than given width if the last skipped
 * character was double width or even 3 more if the last skipped
 * character was invalid (<xx>).
 *
 * Returns number of bytes skipped.
 */
size_t u_skip_chars(const char *str, int *width);

#endif
