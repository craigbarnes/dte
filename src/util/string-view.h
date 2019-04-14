#ifndef UTIL_STRING_VIEW_H
#define UTIL_STRING_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
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

#define string_view_has_literal_prefix(sv, prefix) ( \
    string_view_has_prefix((sv), (prefix), STRLEN(prefix)) \
)

#define string_view_has_literal_prefix_icase(sv, prefix) ( \
    string_view_has_prefix_icase((sv), (prefix), STRLEN(prefix)) \
)

static inline PURE StringView string_view(const char *str, size_t length)
{
    return (StringView) {
        .data = str,
        .length = length
    };
}

static inline PURE StringView string_view_from_cstring(const char *str)
{
    return (StringView) {
        .data = str,
        .length = str ? strlen(str) : 0
    };
}

PURE NONNULL_ARGS
static inline bool string_view_equal(const StringView *a, const StringView *b)
{
    return a->length == b->length && memcmp(a->data, b->data, a->length) == 0;
}

PURE NONNULL_ARGS
static inline bool string_view_equal_strn (
    const StringView *sv,
    const char *str,
    size_t len
) {
    return len == sv->length && memcmp(sv->data, str, len) == 0;
}

PURE NONNULL_ARGS
static inline bool string_view_equal_cstr(const StringView *sv, const char *str)
{
    return string_view_equal_strn(sv, str, strlen(str));
}

PURE NONNULL_ARGS
static inline bool string_view_has_prefix (
    const StringView *sv,
    const char *str,
    size_t length
) {
    return sv->length >= length && memcmp(sv->data, str, length) == 0;
}

PURE NONNULL_ARGS
static inline bool string_view_has_prefix_icase (
    const StringView *sv,
    const char *str,
    size_t length
) {
    return sv->length >= length && strncasecmp(sv->data, str, length) == 0;
}

#endif
