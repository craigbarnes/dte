#ifndef STRING_VIEW_H
#define STRING_VIEW_H

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

bool string_view_equal(const StringView *a, const StringView *b) PURE NONNULL_ARGS;
bool string_view_equal_cstr(const StringView *sv, const char *cstr) PURE NONNULL_ARGS;

#endif
