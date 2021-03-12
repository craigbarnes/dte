#include <stdbool.h>
#include <stdint.h>
#include "utf8.h"
#include "ascii.h"
#include "debug.h"

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
    return seq_len_table[first_byte];
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

CodePoint u_prev_char(const unsigned char *buf, size_t *idx)
{
    size_t i = *idx;
    unsigned char ch = buf[--i];
    if (likely(ch < 0x80)) {
        *idx = i;
        return (CodePoint)ch;
    }

    if (!u_is_continuation_byte(ch)) {
        goto invalid;
    }

    CodePoint u = ch & 0x3f;
    for (unsigned int count = 1, shift = 6; i > 0; ) {
        ch = buf[--i];
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
    u = buf[*idx];
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
    return u_get_nonascii(str, i + 4, idx);
}

CodePoint u_get_char(const unsigned char *buf, size_t size, size_t *idx)
{
    size_t i = *idx;
    CodePoint u = buf[i];
    if (likely(u < 0x80)) {
        *idx = i + 1;
        return u;
    }
    return u_get_nonascii(buf, size, idx);
}

CodePoint u_get_nonascii(const unsigned char *buf, size_t size, size_t *idx)
{
    size_t i = *idx;
    unsigned int first = buf[i++];
    int len = u_seq_len(first);
    if (unlikely(len < 2 || len > size - i + 1)) {
        goto invalid;
    }

    CodePoint u = first & u_get_first_byte_mask(len);
    int c = len - 1;
    do {
        unsigned char ch = buf[i++];
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

void u_set_char_raw(char *str, size_t *idx, CodePoint u)
{
    static const uint8_t prefixes[] = {0, 0, 0xC0, 0xE0, 0xF0};
    const size_t len = u_char_size(u);
    const uint8_t first_byte_prefix = prefixes[len];
    const size_t i = *idx;

    switch (len) {
    case 4:
        str[i + 3] = (u & 0x3F) | 0x80;
        u >>= 6;
        // Fallthrough
    case 3:
        str[i + 2] = (u & 0x3F) | 0x80;
        u >>= 6;
        // Fallthrough
    case 2:
        str[i + 1] = (u & 0x3F) | 0x80;
        u >>= 6;
        // Fallthrough
    case 1:
        str[i] = (u & 0xFF) | first_byte_prefix;
        *idx = i + len;
        break;
    default:
        BUG("unexpected UTF-8 sequence length: %zu", len);
    }
}

void u_set_char(char *str, size_t *idx, CodePoint u)
{
    size_t i = *idx;
    if (likely(u <= 0x7F)) {
        if (unlikely(ascii_iscntrl(u))) {
            // Use caret notation for control chars:
            str[i++] = '^';
            u = (u + 64) & 0x7F;
        }
        str[i++] = u;
        *idx = i;
    } else if (u_is_unprintable(u)) {
        u_set_hex(str, idx, u);
    } else {
        BUG_ON(u > 0x10FFFF); // (implied by !u_is_unprintable(u))
        u_set_char_raw(str, idx, u);
    }
}

void u_set_hex(char *str, size_t *idx, CodePoint u)
{
    static const char hex_tab[16] = "0123456789abcdef";
    char *p = str + *idx;
    p[0] = '<';
    if (!u_is_unicode(u)) {
        // Invalid byte (negated)
        u *= -1;
        p[1] = hex_tab[(u >> 4) & 0x0f];
        p[2] = hex_tab[u & 0x0f];
    } else {
        p[1] = '?';
        p[2] = '?';
    }
    p[3] = '>';
    *idx += 4;
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
