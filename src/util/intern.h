#ifndef UTIL_INTERN_H
#define UTIL_INTERN_H

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

#endif
