#ifndef UTIL_STRING_VIEW_H
#define UTIL_STRING_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include "macros.h"

// A non-owning, length-bounded "view" into another string, similar to
// the C++17 string_view class. The .data member will usually *not* be
// null-terminated and the underlying string *must* outlive the view.
typedef struct {
    const char *data;
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

#define string_view_has_literal_prefix(sv, prefix) ( \
    string_view_has_prefix((sv), (prefix), STRLEN(prefix)) \
)

static inline StringView string_view(const char *str, size_t length)
{
    return (StringView) {
        .data = str,
        .length = length
    };
}

static inline bool string_view_has_prefix(StringView *sv, const char *str, size_t length)
{
    return sv->length >= length && memcmp(sv->data, str, length) == 0;
}

bool string_view_equal(const StringView *a, const StringView *b) PURE NONNULL_ARGS;
bool string_view_equal_cstr(const StringView *sv, const char *cstr) PURE NONNULL_ARGS;

#endif
