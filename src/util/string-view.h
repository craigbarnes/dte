#ifndef UTIL_STRING_VIEW_H
#define UTIL_STRING_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "ascii.h"
#include "macros.h"

// A non-owning, length-bounded "view" into another string, similar to
// the C++17 string_view class. The .data member will usually *not* be
// null-terminated and the underlying string *must* outlive the view.
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
    return (StringView) {
        .data = str,
        .length = str ? strlen(str) : 0
    };
}

NONNULL_ARGS
static inline bool strview_equal(const StringView *a, const StringView *b)
{
    return a->length == b->length && memcmp(a->data, b->data, a->length) == 0;
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
static inline bool strview_has_prefix(const StringView *sv, const char *p)
{
    size_t length = strlen(p);
    return sv->length >= length && memcmp(sv->data, p, length) == 0;
}

NONNULL_ARGS
static inline bool strview_has_prefix_icase(const StringView *sv, const char *p)
{
    size_t length = strlen(p);
    return sv->length >= length && mem_equal_icase(sv->data, p, length);
}

NONNULL_ARGS
static inline bool strview_isblank(const StringView *sv)
{
    const unsigned char *data = sv->data;
    const size_t len = sv->length;
    size_t i = 0;
    while (i < len && ascii_isblank(data[i])) {
        i++;
    }
    return (i == len);
}

NONNULL_ARGS
static inline const unsigned char *strview_memchr(const StringView *sv, int c)
{
    return memchr(sv->data, c, sv->length);
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
static inline void strview_trim_left(StringView *sv)
{
    const unsigned char *data = sv->data;
    const size_t len = sv->length;
    size_t i = 0;
    while (i < len && ascii_isblank(data[i])) {
        i++;
    }
    sv->data = data + i;
    sv->length = len - i;
}

#endif
