#ifndef UTIL_STRING_VIEW_H
#define UTIL_STRING_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include "ascii.h"
#include "debug.h"
#include "macros.h"

// A non-owning, length-bounded "view" into another string, similar to
// the C++17 string_view class or what many languages call a "slice".
// The .data member will usually *not* be null-terminated and the
// underlying string *must* outlive the view.
typedef struct {
    const unsigned char NONSTRING *data;
    size_t length;
} StringView;

#define STRING_VIEW_INIT { \
    .data = NULL, \
    .length = 0 \
}

#define STRING_VIEW(s) { \
    .data = s, \
    .length = STRLEN(s) \
}

static inline StringView string_view(const char *str, size_t length)
{
    return (StringView) {
        .data = str,
        .length = length
    };
}

static inline StringView strview_from_cstring(const char *str)
{
    return string_view(str, str ? strlen(str) : 0);
}

NONNULL_ARGS
static inline bool strview_equal(const StringView *a, const StringView *b)
{
    size_t n = a->length;
    return n == b->length && (n == 0 || memcmp(a->data, b->data, n) == 0);
}

NONNULL_ARGS
static inline bool strview_equal_strn (
    const StringView *sv,
    const char *str,
    size_t len
) {
    return len == sv->length && memcmp(sv->data, str, len) == 0;
}

NONNULL_ARGS
static inline bool strview_equal_strn_icase (
    const StringView *sv,
    const char *str,
    size_t len
) {
    return len == sv->length && mem_equal_icase(sv->data, str, len);
}

NONNULL_ARGS
static inline bool strview_equal_cstring(const StringView *sv, const char *str)
{
    return strview_equal_strn(sv, str, strlen(str));
}

NONNULL_ARGS
static inline bool strview_equal_cstring_icase(const StringView *sv, const char *str)
{
    return strview_equal_strn_icase(sv, str, strlen(str));
}

NONNULL_ARGS
static inline bool strview_has_strn_prefix(const StringView *sv, const char *p, size_t n)
{
    return sv->length >= n && memcmp(sv->data, p, n) == 0;
}

NONNULL_ARGS
static inline bool strview_has_prefix(const StringView *sv, const char *p)
{
    return strview_has_strn_prefix(sv, p, strlen(p));
}

NONNULL_ARGS
static inline bool strview_has_prefix_icase(const StringView *sv, const char *p)
{
    size_t length = strlen(p);
    return sv->length >= length && mem_equal_icase(sv->data, p, length);
}

NONNULL_ARGS
static inline bool strview_has_suffix(const StringView *sv, const char *suf)
{
    size_t len = sv->length;
    size_t suflen = strlen(suf);
    return len >= suflen && memcmp(sv->data + len - suflen, suf, suflen) == 0;
}

NONNULL_ARGS
static inline bool strview_isblank(const StringView *sv)
{
    size_t len = sv->length;
    return len == ascii_blank_prefix_length(sv->data, len);
}

NONNULL_ARGS
static inline bool strview_contains_char_type(const StringView *sv, AsciiCharType mask)
{
    const unsigned char *data = sv->data;
    for (size_t i = 0, n = sv->length; i < n; i++) {
        if (ascii_test(data[i], mask)) {
            return true;
        }
    }
    return false;
}

NONNULL_ARGS
static inline const unsigned char *strview_memchr(const StringView *sv, int c)
{
    return sv->length ? memchr(sv->data, c, sv->length) : NULL;
}

NONNULL_ARGS
static inline const unsigned char *strview_memrchr(const StringView *sv, int c)
{
    const unsigned char *s = sv->data;
    size_t n = sv->length;
    c = (int)(unsigned char)c;
    while (n--) {
        if (s[n] == c) {
            return (s + n);
        }
    }
    return NULL;
}

NONNULL_ARGS
static inline ssize_t strview_memrchr_idx(const StringView *sv, int c)
{
    const unsigned char *ptr = strview_memrchr(sv, c);
    return ptr ? (ssize_t)(ptr - sv->data) : -1;
}

static inline void strview_remove_prefix(StringView *sv, size_t len)
{
    BUG_ON(len > sv->length);
    if (len > 0) {
        sv->data += len;
        sv->length -= len;
    }
}

NONNULL_ARGS
static inline size_t strview_trim_left(StringView *sv)
{
    size_t blank_len = ascii_blank_prefix_length(sv->data, sv->length);
    strview_remove_prefix(sv, blank_len);
    return blank_len;
}

NONNULL_ARGS
static inline void strview_trim_right(StringView *sv)
{
    const unsigned char *data = sv->data;
    size_t n = sv->length;
    while (n && ascii_isblank(data[n - 1])) {
        n--;
    }
    sv->length = n;
}

NONNULL_ARGS
static inline void strview_trim(StringView *sv)
{
    strview_trim_left(sv);
    strview_trim_right(sv);
}

#endif
