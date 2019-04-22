#include "utf8.h"
#include "ascii.h"

static inline CONST_FN int u_seq_len(unsigned int first_byte)
{
    if (first_byte < 0x80) {
        return 1;
    }
    if (first_byte < 0xc0) {
        return 0;
    }
    if (first_byte < 0xe0) {
        return 2;
    }
    if (first_byte < 0xf0) {
        return 3;
    }

    // Could be 0xf8 but RFC 3629 doesn't allow codepoints above 0x10ffff
    if (first_byte < 0xf5) {
        return 4;
    }
    return -1;
}

static inline CONST_FN bool u_is_continuation(CodePoint uch)
{
    return (uch & 0xc0) == 0x80;
}

static inline CONST_FN bool u_seq_len_ok(CodePoint uch, int len)
{
    return u_char_size(uch) == len;
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
static inline CONST_FN unsigned int u_get_first_byte_mask(unsigned int len)
{
    return (1U << 7U >> len) - 1;
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
    unsigned int count, shift;
    CodePoint u;

    u = buf[--i];
    if (u < 0x80) {
        *idx = i;
        return u;
    }

    if (!u_is_continuation(u)) {
        goto invalid;
    }

    u &= 0x3f;
    count = 1;
    shift = 6;
    while (i) {
        unsigned int ch = buf[--i];
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
    if (u < 0x80) {
        *idx = i + 1;
        return u;
    }
    return u_get_nonascii(str, i + 4, idx);
}

CodePoint u_get_char(const unsigned char *buf, size_t size, size_t *idx)
{
    size_t i = *idx;
    CodePoint u = buf[i];
    if (u < 0x80) {
        *idx = i + 1;
        return u;
    }
    return u_get_nonascii(buf, size, idx);
}

CodePoint u_get_nonascii(const unsigned char *buf, size_t size, size_t *idx)
{
    size_t i = *idx;
    int len, c;
    unsigned int first, u;

    first = buf[i++];
    len = u_seq_len(first);
    if (unlikely(len < 2 || len > size - i + 1)) {
        goto invalid;
    }

    u = first & u_get_first_byte_mask(len);
    c = len - 1;
    do {
        CodePoint ch = buf[i++];
        if (!u_is_continuation(ch)) {
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

void u_set_char_raw(char *str, size_t *idx, CodePoint uch)
{
    size_t i = *idx;
    if (uch <= 0x7f) {
        str[i++] = uch;
    } else if (uch <= 0x7ff) {
        str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 0] = uch | 0xc0;
        i += 2;
    } else if (uch <= 0xffff) {
        str[i + 2] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 0] = uch | 0xe0;
        i += 3;
    } else if (uch <= 0x10ffff) {
        str[i + 3] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 2] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 0] = uch | 0xf0;
        i += 4;
    } else {
        // Invalid byte value
        str[i++] = uch & 0xff;
    }
    *idx = i;
}

void u_set_char(char *str, size_t *idx, CodePoint uch)
{
    size_t i = *idx;
    if (uch < 0x80) {
        if (ascii_iscntrl(uch)) {
            u_set_ctrl(str, idx, uch);
        } else {
            str[i++] = uch;
            *idx = i;
        }
    } else if (u_is_unprintable(uch)) {
        u_set_hex(str, idx, uch);
    } else if (uch <= 0x7ff) {
        str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 0] = uch | 0xc0;
        i += 2;
        *idx = i;
    } else if (uch <= 0xffff) {
        str[i + 2] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 0] = uch | 0xe0;
        i += 3;
        *idx = i;
    } else if (uch <= 0x10ffff) {
        str[i + 3] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 2] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 1] = (uch & 0x3f) | 0x80; uch >>= 6;
        str[i + 0] = uch | 0xf0;
        i += 4;
        *idx = i;
    }
}

void u_set_hex(char *str, size_t *idx, CodePoint uch)
{
    static const char hex_tab[16] = "0123456789abcdef";
    char *p = str + *idx;
    p[0] = '<';
    if (!u_is_unicode(uch)) {
        // Invalid byte (negated)
        uch *= -1;
        p[1] = hex_tab[(uch >> 4) & 0x0f];
        p[2] = hex_tab[uch & 0x0f];
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

static bool has_prefix(const char *str, const char *prefix_lcase)
{
    size_t ni = 0;
    size_t hi = 0;
    CodePoint pc;
    while ((pc = u_str_get_char(prefix_lcase, &ni))) {
        CodePoint sc = u_str_get_char(str, &hi);
        if (sc != pc && u_to_lower(sc) != pc) {
            return false;
        }
    }
    return true;
}

ssize_t u_str_index(const char *haystack, const char *needle_lcase)
{
    size_t hi = 0;
    size_t ni = 0;
    CodePoint nc = u_str_get_char(needle_lcase, &ni);

    if (!nc) {
        return 0;
    }

    while (haystack[hi]) {
        size_t prev = hi;
        CodePoint hc = u_str_get_char(haystack, &hi);
        if (
            (hc == nc || u_to_lower(hc) == nc)
            && has_prefix(haystack + hi, needle_lcase + ni)
        ) {
            return prev;
        }
    }
    return -1;
}
