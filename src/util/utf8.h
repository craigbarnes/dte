#ifndef UTIL_UTF8_H
#define UTIL_UTF8_H

#include <stddef.h>
#include "debug.h"
#include "macros.h"
#include "unicode.h"

enum {
    // Longest UTF-8 sequence (in bytes) permitted by RFC 3629
    // (maximum number of bytes written by u_set_char_raw())
    UTF8_MAX_SEQ_LEN = 4, // STRLEN(u8"\U0001F44D")

    // Number of bytes written by u_set_hex()
    U_SET_HEX_LEN = 4, // STRLEN("<ff>")

    // Maximum number of bytes written by u_set_char()
    U_SET_CHAR_MAXLEN = 4, // MAX(UTF8_MAX_SEQ_LEN, U_SET_HEX_LEN)
};

typedef enum {
    // Replace C0 control characters with Unicode "control picture"
    // symbols, instead of using caret notation (which can become
    // quite ambuguous when formatting terminal escape sequences)
    MPF_C0_SYMBOLS = 1 << 0,
} MakePrintableFlags;

size_t u_str_width(const unsigned char *str);
size_t u_skip_chars(const char *str, int *width);
CodePoint u_prev_char(const unsigned char *str, size_t *idx);
CodePoint u_str_get_char(const unsigned char *str, size_t *idx);
CodePoint u_get_char(const unsigned char *str, size_t size, size_t *idx);
CodePoint u_get_nonascii(const unsigned char *str, size_t size, size_t *idx);
size_t u_set_char_raw(char *buf, CodePoint u);
size_t u_set_char(char *buf, CodePoint u);
size_t u_set_hex(char buf[U_SET_HEX_LEN], CodePoint u);

// Return the number of bytes needed to encode Unicode codepoint `u`
// in UTF-8, or 1 for codepoints exceeding UNICODE_MAX_VALID_CODEPOINT.
// Those in the latter category may have originated as values returned
// by u_get_nonascii() or u_prev_char() (i.e. invalid bytes in a
// sequence that have been negated).
static inline size_t u_char_size(CodePoint u)
{
    // If `u` is invalid, set `adj` to 3 and use to adjust the calculation
    // so that 1 is returned
    size_t inv = (u > UNICODE_MAX_VALID_CODEPOINT);
    size_t adj = inv | (inv << 1);

    return 1 + (u > 0x7F) + (u > 0x7FF) + (u > 0xFFFF) - adj;
}

static inline size_t u_make_printable (
    const char *restrict src,
    size_t src_len,
    char *restrict dest,
    size_t dest_len,
    MakePrintableFlags flags
) {
    BUG_ON(dest_len == 0);
    size_t len = 0;

    for (size_t i = 0; i < src_len && len + U_SET_CHAR_MAXLEN < dest_len; ) {
        CodePoint u = u_get_char(src, src_len, &i);
        if (flags & MPF_C0_SYMBOLS) {
            u = (u < 0x20) ? u + 0x2400 : (u == 0x7F ? 0x2421 : u);
        }
        len += u_set_char(dest + len, u);
    }

    dest[len] = '\0';
    return len;
}

#endif
