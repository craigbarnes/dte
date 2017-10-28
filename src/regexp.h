#ifndef REGEXP_H
#define REGEXP_H

#include <stdbool.h>
#include <regex.h>
#include "ptr-array.h"

bool regexp_match_nosub(const char *pattern, const char *buf, long size);
bool regexp_match(const char *pattern, const char *buf, long size, PointerArray *m);

bool regexp_compile_internal(regex_t *re, const char *pattern, int flags);
bool regexp_exec(const regex_t *re, const char *buf, long size, long nr_m, regmatch_t *m, int flags);
bool regexp_exec_sub(const regex_t *re, const char *buf, long size, PointerArray *matches, int flags);

static inline bool regexp_compile(regex_t *re, const char *pattern, int flags)
{
    return regexp_compile_internal(re, pattern, flags | REG_EXTENDED);
}

static inline bool regexp_compile_basic(regex_t *re, const char *pattern, int flags)
{
    return regexp_compile_internal(re, pattern, flags);
}

#endif
