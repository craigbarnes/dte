#ifndef REGEXP_H
#define REGEXP_H

#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include "error.h"
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
    const char *str;
    regex_t re;
} InternedRegexp;

// Platform-specific patterns for matching word boundaries, as detected
// and initialized by regexp_init_word_boundary_tokens()
typedef struct {
    char start[8];
    char end[8];
} RegexpWordBoundaryTokens;

void regexp_compile_or_fatal_error(regex_t *re, const char *pattern, int flags) NONNULL_ARGS;
bool regexp_init_word_boundary_tokens(RegexpWordBoundaryTokens *rwbt) NONNULL_ARGS;
bool regexp_error_msg(ErrorBuffer *ebuf, const regex_t *re, const char *pattern, int err) NONNULL_ARG(2, 3);
char *regexp_escape(const char *pattern, size_t len) NONNULL_ARGS WARN_UNUSED_RESULT;
size_t regexp_escapeb(char *buf, size_t buflen, const char *pat, size_t plen) NONNULL_ARG(1);

const InternedRegexp *regexp_intern(ErrorBuffer *ebuf, const char *pattern) NONNULL_ARG(2) WARN_UNUSED_RESULT;
bool regexp_is_interned(const char *pattern) NONNULL_ARGS;
void free_interned_regexps(void);

WARN_UNUSED_RESULT NONNULL_ARGS
bool regexp_exec (
    const regex_t *re,
    const char *buf,
    size_t size,
    size_t nmatch,
    regmatch_t *pmatch,
    int flags
);

WARN_UNUSED_RESULT NONNULL_ARG(2, 3)
static inline bool regexp_compile(ErrorBuffer *ebuf, regex_t *re, const char *pattern, int flags)
{
    int err = regcomp(re, pattern, flags | DEFAULT_REGEX_FLAGS);
    return !err || regexp_error_msg(ebuf, re, pattern, err);
}

WARN_UNUSED_RESULT NONNULL_ARG(2)
static inline bool regexp_is_valid(ErrorBuffer *ebuf, const char *pattern, int flags)
{
    regex_t re;
    if (!regexp_compile(ebuf, &re, pattern, flags | REG_NOSUB)) {
        return false;
    }
    regfree(&re);
    return true;
}

#endif
