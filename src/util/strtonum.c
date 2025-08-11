#include <errno.h>
#include <string.h>
#include "strtonum.h"
#include "arith.h"
#include "ascii.h"
#include "debug.h"
#include "xmemrchr.h"
#include "xstring.h"

enum {
    A = 0xA, B = 0xB, C = 0xC,
    D = 0xD, E = 0xE, F = 0xF,
    I = HEX_INVALID
};

// Indices are offset by 48 ('0'; 0x30)
const uint8_t hex_decode_table[64] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, I, I, I, I, I, I, // 0x30  0123456789......
    I, A, B, C, D, E, F, I, I, I, I, I, I, I, I, I, // 0x40  .ABCDEF.........
    I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, // 0x50  ................
    I, A, B, C, D, E, F, I, I, I, I, I, I, I, I, I, // 0x60  .abcdef.........
};

UNITTEST {
    static_assert((I & 0xF) == 0);
    BUG_ON(hex_decode_table[sizeof(hex_decode_table) - 1] != I);
}

size_t buf_parse_uintmax(const char *str, size_t len, uintmax_t *valp)
{
    if (unlikely(len == 0 || !ascii_isdigit(str[0]))) {
        return 0;
    }

    uintmax_t val = str[0] - '0';
    size_t i = 1;

    while (i < len && ascii_isdigit(str[i])) {
        if (unlikely(umax_multiply_overflows(val, 10, &val))) {
            return 0;
        }
        if (unlikely(umax_add_overflows(val, str[i++] - '0', &val))) {
            return 0;
        }
    }

    *valp = val;
    return i;
}

size_t buf_parse_ulong(const char *str, size_t len, unsigned long *valp)
{
    uintmax_t val;
    size_t n = buf_parse_uintmax(str, len, &val);
    if (n == 0 || val > ULONG_MAX) {
        return 0;
    }
    *valp = (unsigned long)val;
    return n;
}

size_t buf_parse_uint(const char *str, size_t len, unsigned int *valp)
{
    uintmax_t val;
    size_t n = buf_parse_uintmax(str, len, &val);
    if (n == 0 || val > UINT_MAX) {
        return 0;
    }
    *valp = (unsigned int)val;
    return n;
}

size_t buf_parse_size(const char *str, size_t len, size_t *valp)
{
    uintmax_t val;
    size_t n = buf_parse_uintmax(str, len, &val);
    if (n == 0 || val > SIZE_MAX) {
        return 0;
    }
    *valp = (size_t)val;
    return n;
}

static size_t buf_parse_long(const char *str, size_t len, long *valp)
{
    if (unlikely(len == 0)) {
        return 0;
    }

    bool negative = false;
    size_t skipped = 0;
    switch (str[0]) {
    case '-':
        negative = true;
        // Fallthrough
    case '+':
        skipped = 1;
        str++;
        len--;
        break;
    }

    uintmax_t val;
    size_t n = buf_parse_uintmax(str, len, &val);
    if (n == 0 || val > LONG_MAX) {
        return 0;
    }

    if (negative) {
        *valp = -((long)val);
    } else {
        *valp = (long)val;
    }

    return n + skipped;
}

bool str_to_int(const char *str, int *valp)
{
    const size_t len = strlen(str);
    if (unlikely(len == 0)) {
        return false;
    }
    long val;
    const size_t n = buf_parse_long(str, len, &val);
    if (n != len || val < INT_MIN || val > INT_MAX) {
        return false;
    }
    *valp = (int)val;
    return true;
}

static bool str_to_uintmax(const char *str, uintmax_t *valp)
{
    const size_t len = strlen(str);
    if (unlikely(len == 0)) {
        return false;
    }
    uintmax_t val;
    const size_t n = buf_parse_uintmax(str, len, &val);
    if (n != len) {
        return false;
    }
    *valp = val;
    return true;
}

bool str_to_uint(const char *str, unsigned int *valp)
{
    uintmax_t x;
    if (!str_to_uintmax(str, &x) || x > UINT_MAX) {
        return false;
    }
    *valp = (unsigned int)x;
    return true;
}

bool str_to_ulong(const char *str, unsigned long *valp)
{
    uintmax_t x;
    if (!str_to_uintmax(str, &x) || x > ULONG_MAX) {
        return false;
    }
    *valp = (unsigned long)x;
    return true;
}

bool str_to_size(const char *str, size_t *valp)
{
    uintmax_t x;
    if (!str_to_uintmax(str, &x) || x > SIZE_MAX) {
        return false;
    }
    *valp = (size_t)x;
    return true;
}

// Parse line and column number from line[,col] or line[:col]
bool str_to_xfilepos(const char *str, size_t *linep, size_t *colp)
{
    size_t len = strlen(str);
    size_t line, col;
    size_t i = buf_parse_size(str, len, &line);
    if (i == 0 || line < 1) {
        return false;
    }

    // If an explicit column wasn't specified in `str`, set *colp to 0
    // (which is NOT a valid column number). Callers should be prepared
    // to check this and substitute it for something more appropriate,
    // or otherwise use str_to_filepos() instead.
    if (i == len) {
        col = 0;
        goto out;
    }

    char c = str[i];
    if ((c != ':' && c != ',') || !str_to_size(str + i + 1, &col) || col < 1) {
        return false;
    }

out:
    *linep = line;
    *colp = col;
    return true;
}

// This is much like str_to_xfilepos(), except *colp is set to 1 if no
// explicit column number is specified in `str`. This is a convenience
// to callers that want a valid column number (when omitted) and don't
// need to do anything different for e.g. "32" vs. "32:1".
bool str_to_filepos(const char *str, size_t *linep, size_t *colp)
{
    size_t col;
    bool r = str_to_xfilepos(str, linep, &col);
    if (r) {
        *colp = col + !col;
    }
    return r;
}

// Parse file:line[:col] format (e.g. "dir/filename.ext:12:45") and return
// the `file` part if successful, or a zero-length StringView on failure.
// Note that `:line` isn't optional.
StringView parse_file_line_col(const char *str, size_t *linep, size_t *colp)
{
    const char *a = strrchr(str, ':');
    const char *b = a ? xmemrchr(str, ':', a - str) : NULL;
    const char *c = b ? b : a;
    bool ok = (c && c != str && str_to_filepos(c + 1, linep, colp));
    return string_view(str, ok ? c - str : 0);
}

// Return the number of decimal digits in `x`
size_t size_str_width(size_t x)
{
    size_t width = 0;
    do {
        x /= 10;
        width++;
    } while (x);
    return width;
}

// Convert a string of decimal digits with an optional KiB/MiB/GiB/etc.
// suffix to an intmax_t value representing the number of bytes, or a
// negated <errno.h> value in the case of errors
intmax_t parse_filesize(const char *str)
{
    uintmax_t x;
    size_t ndigits = buf_parse_uintmax(str, strlen(str), &x);
    if (unlikely(ndigits == 0 || x > INTMAX_MAX)) {
        return ascii_isdigit(str[0]) ? -EOVERFLOW : -EINVAL;
    }

    const char *suffix = str + ndigits;
    unsigned int shift;
    switch (suffix[0]) {
    case 'K': shift = 10; break;
    case 'M': shift = 20; break;
    case 'G': shift = 30; break;
    case 'T': shift = 40; break;
    case 'P': shift = 50; break;
    case 'E': shift = 60; break;
    case '\0': return x;
    default: return -EINVAL;
    }

    if (unlikely(!streq(suffix + 1, "iB"))) {
        return -EINVAL;
    }

    uintmax_t val = x << shift;
    if (unlikely(val >> shift != x || x > INTMAX_MAX)) {
        return -EOVERFLOW;
    }

    return val;
}
