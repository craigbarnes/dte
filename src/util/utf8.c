#include <stdbool.h>
#include <stdint.h>
#include "utf8.h"
#include "ascii.h"
#include "debug.h"
#include "numtostr.h"

enum {
    I = -1, // Invalid byte
    C = 0,  // Continuation byte
};

static const int8_t seq_len_table[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 00..0F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 10..1F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 20..2F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 30..3F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 40..4F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 50..5F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 60..6F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 70..7F
    C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, // 80..8F
    C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, // 90..9F
    C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, // A0..AF
    C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, C, // B0..BF
    I, I, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C0..CF
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // D0..DF
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // E0..EF
    4, 4, 4, 4, 4, I, I, I, I, I, I, I, I, I, I, I  // F0..FF
};

static int u_seq_len(unsigned char first_byte)
{
    int8_t len = seq_len_table[first_byte];
    BUG_ON(len < I || len > UTF8_MAX_SEQ_LEN);
    return len;
}

static bool u_is_continuation_byte(unsigned char u)
{
    return (u & 0xc0) == 0x80;
}

static bool u_seq_len_ok(CodePoint u, int len)
{
    return u_char_size(u) == len;
}

/*
 * Len  Mask         Note
 * -------------------------------------------------
 * 1    0111 1111    Not supported by this function!
 * 2    0001 1111
 * 3    0000 1111
 * 4    0000 0111
 * 5    0000 0011    Forbidden by RFC 3629
 * 6    0000 0001    Forbidden by RFC 3629
 */
static unsigned int u_get_first_byte_mask(unsigned int len)
{
    BUG_ON(len < 2);
    BUG_ON(len > UTF8_MAX_SEQ_LEN);
    return (0x80 >> len) - 1;
}

size_t u_str_width(const unsigned char *str)
{
    size_t i = 0, w = 0;
    while (str[i]) {
        w += u_char_width(u_str_get_char(str, &i));
    }
    return w;
}

CodePoint u_prev_char(const unsigned char *str, size_t *idx)
{
    size_t i = *idx;
    unsigned char ch = str[--i];
    if (likely(ch < 0x80)) {
        *idx = i;
        return (CodePoint)ch;
    }

    if (!u_is_continuation_byte(ch)) {
        goto invalid;
    }

    CodePoint u = ch & 0x3f;
    for (unsigned int count = 1, shift = 6; i > 0; ) {
        ch = str[--i];
        unsigned int len = u_seq_len(ch);
        count++;
        if (len == 0) {
            if (count == 4) {
                // Too long sequence
                break;
            }
            u |= (ch & 0x3f) << shift;
            shift += 6;
        } else if (count != len) {
            // Incorrect length
            break;
        } else {
            u |= (ch & u_get_first_byte_mask(len)) << shift;
            if (!u_seq_len_ok(u, len)) {
                break;
            }
            *idx = i;
            return u;
        }
    }

invalid:
    *idx = *idx - 1;
    u = str[*idx];
    return -u;
}

CodePoint u_str_get_char(const unsigned char *str, size_t *idx)
{
    size_t i = *idx;
    CodePoint u = str[i];
    if (likely(u < 0x80)) {
        *idx = i + 1;
        return u;
    }
    return u_get_nonascii(str, i + UTF8_MAX_SEQ_LEN, idx);
}

CodePoint u_get_char(const unsigned char *str, size_t size, size_t *idx)
{
    size_t i = *idx;
    CodePoint u = str[i];
    if (likely(u < 0x80)) {
        *idx = i + 1;
        return u;
    }
    return u_get_nonascii(str, size, idx);
}

CodePoint u_get_nonascii(const unsigned char *str, size_t size, size_t *idx)
{
    size_t i = *idx;
    unsigned int first = str[i++];
    int len = u_seq_len(first);
    if (unlikely(len < 2 || len > size - i + 1)) {
        goto invalid;
    }

    CodePoint u = first & u_get_first_byte_mask(len);
    int c = len - 1;
    do {
        unsigned char ch = str[i++];
        if (!u_is_continuation_byte(ch)) {
            goto invalid;
        }
        u = (u << 6) | (ch & 0x3f);
    } while (--c);

    if (!u_seq_len_ok(u, len)) {
        goto invalid;
    }

    *idx = i;
    return u;

invalid:
    *idx += 1;
    return -first;
}

size_t u_set_char_raw(char *buf, CodePoint u)
{
    unsigned int prefix = 0;
    size_t len = u_char_size(u);
    BUG_ON(len == 0 || len > UTF8_MAX_SEQ_LEN);

    switch (len) {
    case 4:
        buf[3] = (u & 0x3F) | 0x80;
        u >>= 6;
        prefix |= 0xF0;
        // Fallthrough
    case 3:
        buf[2] = (u & 0x3F) | 0x80;
        u >>= 6;
        prefix |= 0xE0;
        // Fallthrough
    case 2:
        buf[1] = (u & 0x3F) | 0x80;
        u >>= 6;
        prefix |= 0xC0;
    }

    buf[0] = (u & 0xFF) | prefix;
    return len;
}

size_t u_set_char(char *buf, CodePoint u)
{
    if (likely(u <= 0x7F)) {
        size_t i = 0;
        if (unlikely(ascii_iscntrl(u))) {
            // Use caret notation for control chars:
            buf[i++] = '^';
            u = (u + 64) & 0x7F;
        }
        buf[i++] = u;
        return i;
    }

    if (u_is_unprintable(u)) {
        return u_set_hex(buf, u);
    }

    BUG_ON(u > 0x10FFFF); // (implied by !u_is_unprintable(u))
    return u_set_char_raw(buf, u);
}

size_t u_set_hex(char *buf, CodePoint u)
{
    buf[0] = '<';
    if (!u_is_unicode(u)) {
        // Invalid byte (negated)
        u *= -1;
        hex_encode_byte(buf + 1, u & 0xFF);
    } else {
        buf[1] = '?';
        buf[2] = '?';
    }
    buf[3] = '>';
    return 4;
}

size_t u_skip_chars(const char *str, int *width)
{
    int w = *width;
    size_t idx = 0;
    while (str[idx] && w > 0) {
        w -= u_char_width(u_str_get_char(str, &idx));
    }

    // Add 1..3 if skipped 'too much' (the last char was double
    // width or invalid (<xx>))
    *width -= w;
    return idx;
}
