#ifndef UTIL_INTERN_H
#define UTIL_INTERN_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "macros.h"
#include "string-view.h"

const void *mem_intern(const void *data, size_t len) NONNULL_ARGS_AND_RETURN;
void free_interned_strings(void);

static inline const char *str_intern(const char *str)
{
    return mem_intern(str, strlen(str));
}

static inline StringView strview_intern(const char *str)
{
    size_t len = strlen(str);
    return string_view(mem_intern(str, len), len);
}

// Test 2 interned strings for equality, via pointer equality. This
// function exists purely to "self-document" the places in the codebase
// where pointer equality is used as a substitute for streq(). Note
// that particular attention should be given to the special case in
// encoding_from_type() and buffer_set_encoding(), when making changes
// here.
static inline bool interned_strings_equal(const char *a, const char *b)
{
    return a == b;
}

#endif
