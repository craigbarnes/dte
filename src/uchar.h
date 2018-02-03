#ifndef UCHAR_H
#define UCHAR_H

#include <stddef.h>
#include "unicode.h"

static inline CONST_FN size_t u_char_size(CodePoint uch)
{
    if (uch <= UINT32_C(0x7f)) {
        return 1;
    } else if (uch <= UINT32_C(0x7ff)) {
        return 2;
    } else if (uch <= UINT32_C(0xffff)) {
        return 3;
    } else if (uch <= UINT32_C(0x10ffff)) {
        return 4;
    }

    // Invalid byte in UTF-8 byte sequence.
    return 1;
}

static inline void u_set_ctrl(char *buf, size_t *idx, CodePoint u)
{
    size_t i = *idx;
    buf[i++] = '^';
    if (u == 0x7f) {
        buf[i++] = '?';
    } else {
        buf[i++] = u | 0x40;
    }
    *idx = i;
}

unsigned int u_str_width(const unsigned char *str);

unsigned int u_prev_char(const unsigned char *buf, size_t *idx);
unsigned int u_str_get_char(const unsigned char *str, size_t *idx);
unsigned int u_get_char(const unsigned char *buf, size_t size, size_t *idx);
unsigned int u_get_nonascii(const unsigned char *buf, size_t size, size_t *idx);

void u_set_char_raw(char *str, size_t *idx, CodePoint uch);
void u_set_char(char *str, size_t *idx, CodePoint uch);
void u_set_hex(char *str, size_t *idx, CodePoint uch);

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

int u_str_index(const char *haystack, const char *needle_lcase);

#endif
