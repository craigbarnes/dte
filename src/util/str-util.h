#ifndef UTIL_STR_UTIL_H
#define UTIL_STR_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "macros.h"
#include "string-view.h"
#include "xstring.h"

#define copyliteral(dest, lit) copystrn(dest, lit, STRLEN(lit))

static inline size_t copystrn(char *dest, const char *src, size_t len)
{
    memcpy(dest, src, len);
    return len;
}

static inline bool str_is_null_or_empty(const char *str)
{
    return !str || str[0] == '\0';
}

// Like getenv(3) but with `const char*` return type and also returning
// NULL for empty strings
static inline const char *xgetenv(const char *name)
{
    const char *val = getenv(name);
    return str_is_null_or_empty(val) ? NULL : val;
}

PURE NONNULL_ARGS
static inline bool str_has_prefix(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

PURE NONNULL_ARGS
static inline bool str_has_suffix(const char *str, const char *suffix)
{
    size_t l1 = strlen(str);
    size_t l2 = strlen(suffix);
    return (l1 >= l2) && mem_equal(str + l1 - l2, suffix, l2);
}

NONNULL_ARGS
static inline bool strn_has_strview_prefix(const char *s, size_t n, const StringView *p)
{
    return n >= p->length && mem_equal(s, p->data, p->length);
}

NONNULL_ARGS
static inline size_t str_common_prefix_length(const char *a, const char *b)
{
    size_t n = 0;
    while (a[n] && a[n] == b[n]) {
        n++;
    }
    return n;
}

// Replaces all occurrences of a specific byte with a replacement byte
// and returns the length of the string (like strlen(str))
NONNULL_ARGS
static inline size_t str_replace_byte(char *str, char byte, char replacement)
{
    size_t n = 0;
    for (char c; (c = str[n]); n++) {
        if (c == byte) {
            str[n] = replacement;
        }
    }
    return n;
}

NONNULL_ARGS
static inline void strn_replace_byte(char *str, size_t n, char byte, char rep)
{
    for (size_t i = 0; i < n; i++) {
        if (str[i] == byte) {
            str[i] = rep;
        }
    }
}

// Extract a substring between `buf + pos` and either the next `delim`
// byte (if found) or `buf + size` (the remainder of the string). The
// substring is returned as a StringView and the `posp` in-out-param
// is set to the offset one byte after the found delimiter (or to the
// end of the size-bounded string, if no delimiter was found).
NONNULL_ARGS
static inline StringView get_delim(const char *buf, size_t *posp, size_t size, int delim)
{
    size_t pos = *posp;
    BUG_ON(pos >= size);
    size_t len = size - pos;
    const char *start = buf + pos;
    const char *found = memchr(start, delim, len);
    len = found ? (size_t)(found - start) : len;
    size_t delim_len = found ? 1 : 0;
    *posp += len + delim_len;
    return string_view(start, len);
}

// Similar to get_delim(), but returning a null-terminated substring
// instead of a StringView, by mutating the contents of `buf` (i.e.
// replacing the delimiter with a NUL byte). Using get_delim() should
// be preferred, unless the substring specifically needs to be
// null-terminated (e.g. for passing to a library function).
NONNULL_ARGS
static inline char *get_delim_str(char *buf, size_t *posp, size_t size, int delim)
{
    size_t pos = *posp;
    BUG_ON(pos >= size);
    size_t len = size - pos;
    char *start = buf + pos;
    char *found = memchr(start, delim, len);
    if (found) {
        *found = '\0';
        *posp += (size_t)(found - start) + 1;
    } else {
        // If no delimiter is found, write the null-terminator 1 byte
        // beyond the `size` bound. Callers must ensure this is safe
        // to do. Thus, when calling this function (perhaps repeatedly)
        // to consume an entire string, either buf[size-1] must be a
        // delim byte or buf[size] must be in-bounds, writable memory.
        start[len] = '\0';
        *posp += len;
    }
    return start;
}

NONNULL_ARGS
static inline StringView buf_slice_next_line(const char *buf, size_t *posp, size_t size)
{
    return get_delim(buf, posp, size, '\n');
}

NONNULL_ARGS
static inline char *buf_next_line(char *buf, size_t *posp, size_t size)
{
    return get_delim_str(buf, posp, size, '\n');
}

NONNULL_ARGS
static inline size_t count_nl(const char *buf, size_t size)
{
    const char *end = buf + size;
    size_t nl = 0;
    while (buf < end) {
        buf = memchr(buf, '\n', end - buf);
        if (!buf) {
            break;
        }
        buf++;
        nl++;
    }
    return nl;
}

#endif
