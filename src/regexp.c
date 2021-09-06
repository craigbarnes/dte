#include <errno.h>
#include <stdlib.h>
#include "regexp.h"
#include "error.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"

RegexpWordBoundaryTokens regexp_word_boundary;

void regexp_error_msg(const regex_t *re, const char *pattern, int err)
{
    char msg[1024];
    regerror(err, re, msg, sizeof(msg));
    error_msg("%s: %s", msg, pattern);
}

bool regexp_compile_internal(regex_t *re, const char *pattern, int flags)
{
    int err = regcomp(re, pattern, flags);
    if (err) {
        regexp_error_msg(re, pattern, err);
        return false;
    }
    return true;
}

void regexp_compile_or_fatal_error(regex_t *re, const char *pattern, int flags)
{
    int err = regcomp(re, pattern, flags | REG_EXTENDED);
    if (unlikely(err)) {
        char msg[1024];
        regerror(err, re, msg, sizeof(msg));
        fatal_error(msg, EINVAL);
    }
}

bool regexp_exec (
    const regex_t *re,
    const char *buf,
    size_t size,
    size_t nmatch,
    regmatch_t *pmatch,
    int flags
) {
    // "If REG_STARTEND is specified, pmatch must point to at least one
    // regmatch_t (even if nmatch is 0 or REG_NOSUB was specified), to
    // hold the input offsets for REG_STARTEND."
    // -- https://man.openbsd.org/regex.3
    BUG_ON(!pmatch);

// ASan's __interceptor_regexec() doesn't support REG_STARTEND
#if defined(REG_STARTEND) && !defined(ASAN_ENABLED) && !defined(MSAN_ENABLED)
    pmatch[0].rm_so = 0;
    pmatch[0].rm_eo = size;
    return !regexec(re, buf, nmatch, pmatch, flags | REG_STARTEND);
#else
    // Buffer must be null-terminated if REG_STARTEND isn't supported
    char *tmp = xstrcut(buf, size);
    int ret = !regexec(re, tmp, nmatch, pmatch, flags);
    free(tmp);
    return ret;
#endif
}

void regexp_init_word_boundary_tokens(void)
{
    const char text[] = "SSfooEE SSfoo fooEE foo SSfooEE";
    const regoff_t match_start = 20, match_end = 23;
    const RegexpWordBoundaryTokens pairs[] = {
        {"\\<", "\\>"},
        {"[[:<:]]", "[[:>:]]"},
        {"\\b", "\\b"},
    };

    BUG_ON(ARRAY_COUNT(text) <= match_end);
    BUG_ON(!mem_equal(text + match_start - 1, " foo ", 5));

    for (size_t i = 0; i < ARRAY_COUNT(pairs); i++) {
        const char *start = pairs[i].start;
        const char *end = pairs[i].end;
        char patt[32];
        xsnprintf(patt, sizeof(patt), "%s(foo)%s", start, end);
        regex_t re;
        if (regcomp(&re, patt, REG_EXTENDED) != 0) {
            continue;
        }
        regmatch_t m[2];
        bool match = !regexec(&re, text, ARRAY_COUNT(m), m, 0);
        regfree(&re);
        if (match && m[0].rm_so == match_start && m[0].rm_eo == match_end) {
            DEBUG_LOG("regexp word boundary tokens detected: %s %s", start, end);
            regexp_word_boundary = pairs[i];
            break;
        }
    }
}
