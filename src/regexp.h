#ifndef UTIL_REGEXP_H
#define UTIL_REGEXP_H

#include <regex.h>
#include <stdbool.h>
#include "util/ptr-array.h"

bool regexp_match_nosub(const char *pattern, const char *buf, size_t size);
bool regexp_match(const char *pattern, const char *buf, size_t size, PointerArray *m);

bool regexp_compile_internal(regex_t *re, const char *pattern, int flags);
bool regexp_exec(const regex_t *re, const char *buf, size_t size, size_t nr_m, regmatch_t *m, int flags);
bool regexp_exec_sub(const regex_t *re, const char *buf, size_t size, PointerArray *matches, int flags);

static inline bool regexp_compile(regex_t *re, const char *pattern, int flags)
{
    return regexp_compile_internal(re, pattern, flags | REG_EXTENDED);
}

static inline bool regexp_is_valid(const char *pattern, int flags)
{
    regex_t re;
    if (!regexp_compile(&re, pattern, flags | REG_NOSUB)) {
        return false;
    }
    regfree(&re);
    return true;
}

static inline bool regexp_compile_basic(regex_t *re, const char *pattern, int flags)
{
    return regexp_compile_internal(re, pattern, flags);
}

#endif
