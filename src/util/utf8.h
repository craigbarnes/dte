#ifndef UTIL_UTF8_H
#define UTIL_UTF8_H

#include <stddef.h>
#include "macros.h"
#include "unicode.h"

static inline size_t u_char_size(CodePoint u)
{
    if (likely(u <= 0x7f)) {
        return 1;
    } else if (u <= 0x7ff) {
        return 2;
    } else if (u <= 0xffff) {
        return 3;
    } else if (u <= UNICODE_MAX_VALID_CODEPOINT) {
        return 4;
    }

    // Invalid byte in UTF-8 byte sequence
    return 1;
}

size_t u_str_width(const unsigned char *str);

CodePoint u_prev_char(const unsigned char *buf, size_t *idx);
CodePoint u_str_get_char(const unsigned char *str, size_t *idx);
CodePoint u_get_char(const unsigned char *buf, size_t size, size_t *idx);
CodePoint u_get_nonascii(const unsigned char *buf, size_t size, size_t *idx);

void u_set_char_raw(char *str, size_t *idx, CodePoint u);
void u_set_char(char *str, size_t *idx, CodePoint u);
void u_set_hex(char *str, size_t *idx, CodePoint u);

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
