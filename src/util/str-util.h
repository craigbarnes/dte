#ifndef STR_UTIL_H
#define STR_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "macros.h"
#include "xmalloc.h"

#define MEMZERO(ptr) memset((ptr), 0, sizeof(*(ptr)))

NONNULL_ARGS
static inline bool streq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static inline bool xstreq(const char *a, const char *b)
{
    if (a == b) {
        return true;
    } else if (a == NULL || b == NULL) {
        return false;
    }
    return streq(a, b);
}

NONNULL_ARGS
static inline bool str_has_prefix(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

NONNULL_ARGS
static inline bool str_has_suffix(const char *str, const char *suffix)
{
    size_t l1 = strlen(str);
    size_t l2 = strlen(suffix);
    if (l2 > l1) {
        return false;
    }
    return memcmp(str + l1 - l2, suffix, l2) == 0;
}

NONNULL_ARGS
static inline bool mem_equal(const void *s1, const void *s2, size_t n)
{
    return memcmp(s1, s2, n) == 0;
}

NONNULL_ARGS
static inline size_t count_nl(const char *buf, size_t size)
{
    const char *end = buf + size;
    size_t nl = 0;
    while (buf < end) {
        buf = memchr(buf, '\n', end - buf);
        if (!buf) {
            break;
        }
        buf++;
        nl++;
    }
    return nl;
}

static inline char **copy_string_array(char **src, size_t count)
{
    char **dst = xnew(char*, count + 1);
    for (size_t i = 0; i < count; i++) {
        dst[i] = xstrdup(src[i]);
    }
    dst[count] = NULL;
    return dst;
}

NONNULL_ARGS
static inline void free_string_array(char **strings)
{
    for (size_t i = 0; strings[i]; i++) {
        free(strings[i]);
    }
    free(strings);
}

#endif
