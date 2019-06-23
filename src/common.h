#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "util/macros.h"

#define memzero(ptr) memset((ptr), 0, sizeof(*(ptr)))

static inline PURE NONNULL_ARGS bool streq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static inline PURE bool xstreq(const char *a, const char *b)
{
    if (a == b) {
        return true;
    } else if (a == NULL || b == NULL) {
        return false;
    }
    return streq(a, b);
}

static inline PURE NONNULL_ARGS bool str_has_prefix (
    const char *str,
    const char *prefix
) {
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static inline PURE NONNULL_ARGS bool str_has_suffix (
    const char *str,
    const char *suffix
) {
    size_t l1 = strlen(str);
    size_t l2 = strlen(suffix);

    if (l2 > l1) {
        return false;
    }
    return memcmp(str + l1 - l2, suffix, l2) == 0;
}

PURE NONNULL_ARGS
static inline bool mem_equal(const void *s1, const void *s2, size_t n)
{
    return memcmp(s1, s2, n) == 0;
}

size_t count_nl(const char *buf, size_t size);
size_t count_strings(char **strings);
void free_strings(char **strings);

#endif
