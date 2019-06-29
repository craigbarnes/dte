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
    const char NONSTRING *data;
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

#define string_view_equal_literal(sv, str) ( \
    string_view_equal_strn((sv), (str), STRLEN(str)) \
)

#define string_view_equal_literal_icase(sv, str) ( \
    string_view_equal_strn_icase((sv), (str), STRLEN(str)) \
)

#define string_view_has_literal_prefix(sv, prefix) ( \
    string_view_has_prefix((sv), (prefix), STRLEN(prefix)) \
)

#define string_view_has_literal_prefix_icase(sv, prefix) ( \
    string_view_has_prefix_icase((sv), (prefix), STRLEN(prefix)) \
)

static inline StringView string_view(const char *str, size_t length)
{
    return (StringView) {
        .data = str,
        .length = length
    };
}

static inline StringView string_view_from_cstring(const char *str)
{
    return (StringView) {
        .data = str,
        .length = str ? strlen(str) : 0
    };
}

NONNULL_ARGS
static inline bool string_view_equal(const StringView *a, const StringView *b)
{
    return a->length == b->length && memcmp(a->data, b->data, a->length) == 0;
}

NONNULL_ARGS
static inline bool string_view_equal_strn (
    const StringView *sv,
    const char *str,
    size_t len
) {
    return len == sv->length && memcmp(sv->data, str, len) == 0;
}

NONNULL_ARGS
static inline bool string_view_equal_strn_icase (
    const StringView *sv,
    const char *str,
    size_t len
) {
    return len == sv->length && mem_equal_icase(sv->data, str, len);
}

NONNULL_ARGS
static inline bool string_view_equal_cstr(const StringView *sv, const char *str)
{
    return string_view_equal_strn(sv, str, strlen(str));
}

NONNULL_ARGS
static inline bool string_view_has_prefix (
    const StringView *sv,
    const char *str,
    size_t length
) {
    return sv->length >= length && memcmp(sv->data, str, length) == 0;
}

NONNULL_ARGS
static inline bool string_view_has_prefix_icase (
    const StringView *sv,
    const char *str,
    size_t length
) {
    return sv->length >= length && mem_equal_icase(sv->data, str, length);
}

NONNULL_ARGS
static inline void *string_view_memchr(const StringView *sv, int c)
{
    return memchr(sv->data, c, sv->length);
}

NONNULL_ARGS
static inline void *string_view_memrchr(const StringView *sv, int c)
{
    const unsigned char *s = sv->data;
    size_t n = sv->length;
    c = (int)(unsigned char)c;
    while (n--) {
        if (s[n] == c) {
            return (void*)(s + n);
        }
    }
    return NULL;
}

NONNULL_ARGS
static inline void string_view_trim_left(StringView *sv)
{
    const char *data = sv->data;
    const size_t len = sv->length;
    size_t i = 0;
    while (i < len && ascii_isblank(data[i])) {
        i++;
    }
    sv->data = data + i;
    sv->length = len - i;
}

#endif
