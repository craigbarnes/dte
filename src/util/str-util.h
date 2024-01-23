#ifndef UTIL_STR_UTIL_H
#define UTIL_STR_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "macros.h"
#include "string-view.h"
#include "xmalloc.h"

#define memcpy_literal(dest, lit) copystr(dest, lit, STRLEN(lit))

static inline size_t copystr(char *dest, const char *src, size_t len)
{
    memcpy(dest, src, len);
    return len;
}

PURE NONNULL_ARGS
static inline bool streq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

PURE
static inline bool xstreq(const char *a, const char *b)
{
    return (a == b) || (a && b && streq(a, b));
}

PURE NONNULL_ARGS
static inline bool mem_equal(const void *s1, const void *s2, size_t n)
{
    return memcmp(s1, s2, n) == 0;
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

static inline size_t string_array_length(char **strings)
{
    size_t n = 0;
    while (strings[n]) {
        n++;
    }
    return n;
}

static inline bool string_array_contains_prefix(char **strs, const char *prefix)
{
    for (size_t i = 0; strs[i]; i++) {
        if (str_has_prefix(strs[i], prefix)) {
            return true;
        }
    }
    return false;
}

static inline bool string_array_contains_str(char **strs, const char *str)
{
    for (size_t i = 0; strs[i]; i++) {
        if (streq(strs[i], str)) {
            return true;
        }
    }
    return false;
}

static inline char **copy_string_array(char **src, size_t count)
{
    char **dst = xnew(char*, count + 1);
    for (size_t i = 0; i < count; i++) {
        dst[i] = xstrdup(src[i]);
    }
    dst[count] = NULL;
    return dst;
}

NONNULL_ARGS
static inline void free_string_array(char **strings)
{
    for (size_t i = 0; strings[i]; i++) {
        free(strings[i]);
    }
    free(strings);
}

#endif
