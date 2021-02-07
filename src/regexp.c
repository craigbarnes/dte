#include <stdlib.h>
#include "regexp.h"
#include "error.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"

RegexpWordBoundaryTokens regexp_word_boundary;

bool regexp_match_nosub(const char *pattern, const StringView *buf)
{
    regex_t re;
    bool compiled = regexp_compile(&re, pattern, REG_NEWLINE | REG_NOSUB);
    BUG_ON(!compiled);
    regmatch_t m;
    bool ret = regexp_exec(&re, buf->data, buf->length, 1, &m, 0);
    regfree(&re);
    return ret;
}

bool regexp_compile_internal(regex_t *re, const char *pattern, int flags)
{
    int err = regcomp(re, pattern, flags);
    if (err) {
        char msg[1024];
        regerror(err, re, msg, sizeof(msg));
        error_msg("%s: %s", msg, pattern);
        return false;
    }
    return true;
}

bool regexp_exec (
    const regex_t *re,
    const char *buf,
    size_t size,
    size_t nr_m,
    regmatch_t *m,
    int flags
) {
    BUG_ON(!nr_m);
// ASan/MSan don't seem to take REG_STARTEND into account
#if defined(REG_STARTEND) && !defined(ASAN_ENABLED) && !defined(MSAN_ENABLED)
    m[0].rm_so = 0;
    m[0].rm_eo = size;
    return !regexec(re, buf, nr_m, m, flags | REG_STARTEND);
#else
    // Buffer must be null-terminated if REG_STARTEND isn't supported
    char *tmp = xstrcut(buf, size);
    int ret = !regexec(re, tmp, nr_m, m, flags);
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
