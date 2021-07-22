#ifndef REGEXP_H
#define REGEXP_H

#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"
#include "util/string-view.h"

typedef struct {
    char start[8];
    char end[8];
} RegexpWordBoundaryTokens;

extern RegexpWordBoundaryTokens regexp_word_boundary;

bool regexp_compile_internal(regex_t *re, const char *pattern, int flags) WARN_UNUSED_RESULT;

WARN_UNUSED_RESULT
static inline bool regexp_compile(regex_t *re, const char *pattern, int flags)
{
    return regexp_compile_internal(re, pattern, flags | REG_EXTENDED);
}

WARN_UNUSED_RESULT
static inline bool regexp_compile_basic(regex_t *re, const char *pattern, int flags)
{
    return regexp_compile_internal(re, pattern, flags);
}

WARN_UNUSED_RESULT
static inline bool regexp_is_valid(const char *pattern, int flags)
{
    regex_t re;
    if (!regexp_compile(&re, pattern, flags | REG_NOSUB)) {
        return false;
    }
    regfree(&re);
    return true;
}

void regexp_compile_or_fatal_error(regex_t *re, const char *pattern, int flags);
bool regexp_match_nosub(const char *pattern, const StringView *buf) WARN_UNUSED_RESULT;
bool regexp_exec(const regex_t *re, const char *buf, size_t size, size_t nr_m, regmatch_t *m, int flags) WARN_UNUSED_RESULT;
void regexp_init_word_boundary_tokens(void);

#endif
