#ifndef UTIL_INTERN_H
#define UTIL_INTERN_H

#include <stddef.h>
#include <string.h>
#include "macros.h"

NONNULL_ARGS_AND_RETURN
const void *mem_intern(const void *data, size_t len);

NONNULL_ARGS_AND_RETURN
static inline const char *str_intern(const char *str)
{
    return mem_intern(str, strlen(str));
}

#endif
