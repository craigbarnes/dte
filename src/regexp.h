#ifndef REGEXP_H
#define REGEXP_H

#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"

enum {
#ifdef REG_ENHANCED
    // The REG_ENHANCED flag enables various extensions on macOS
    // (see "enhanced features" in re_format(7)). Most of these
    // extensions are enabled by default on Linux (in both glibc
    // and musl) without the need for any extra flags.
    DEFAULT_REGEX_FLAGS = REG_EXTENDED | REG_ENHANCED,
#else
    // POSIX Extended Regular Expressions (ERE) are used almost
    // everywhere in this codebase, except where Basic Regular
    // Expressions (BRE) are explicitly called for (most notably
    // in search_tag(), which is used for ctags patterns).
    DEFAULT_REGEX_FLAGS = REG_EXTENDED,
#endif
};

typedef struct {
    regex_t re;
    char str[];
} CachedRegexp;

typedef struct {
    char *str;
    regex_t re;
} InternedRegexp;

// Platform-specific patterns for matching word boundaries, as detected
// and initialized by regexp_init_word_boundary_tokens()
typedef struct {
    char start[8];
    char end[8];
} RegexpWordBoundaryTokens;

bool regexp_compile_internal(regex_t *re, const char *pattern, int flags) WARN_UNUSED_RESULT;

WARN_UNUSED_RESULT
static inline bool regexp_compile(regex_t *re, const char *pattern, int flags)
{
    return regexp_compile_internal(re, pattern, flags | DEFAULT_REGEX_FLAGS);
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
bool regexp_init_word_boundary_tokens(RegexpWordBoundaryTokens *rwbt);
bool regexp_error_msg(const regex_t *re, const char *pattern, int err);
void free_cached_regexp(CachedRegexp *cr);

const InternedRegexp *regexp_intern(const char *pattern);
bool regexp_is_interned(const char *pattern);
void free_interned_regexps(void);

bool regexp_exec (
    const regex_t *re,
    const char *buf,
    size_t size,
    size_t nmatch,
    regmatch_t *pmatch,
    int flags
) WARN_UNUSED_RESULT;

#endif
