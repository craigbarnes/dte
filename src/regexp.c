#include <string.h>
#include "regexp.h"
#include "debug.h"
#include "error.h"
#include "util/xmalloc.h"

bool regexp_match_nosub(const char *pattern, const char *buf, size_t size)
{
    regex_t re;
    bool compiled = regexp_compile(&re, pattern, REG_NEWLINE | REG_NOSUB);
    BUG_ON(!compiled);
    regmatch_t m;
    bool ret = regexp_exec(&re, buf, size, 1, &m, 0);
    regfree(&re);
    return ret;
}

bool regexp_match (
    const char *pattern,
    const char *buf,
    size_t size,
    PointerArray *m
) {
    regex_t re;
    bool compiled = regexp_compile(&re, pattern, REG_NEWLINE);
    BUG_ON(!compiled);
    bool ret = regexp_exec_sub(&re, buf, size, m, 0);
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
// Clang's address sanitizer seemingly doesn't take REG_STARTEND into
// account when checking for buffer overflow.
#if defined(REG_STARTEND) && !defined(CLANG_ASAN_ENABLED)
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

bool regexp_exec_sub (
    const regex_t *re,
    const char *buf,
    size_t size,
    PointerArray *matches,
    int flags
) {
    regmatch_t m[16];
    bool ret = regexp_exec(re, buf, size, ARRAY_COUNT(m), m, flags);
    if (!ret) {
        return false;
    }
    for (size_t i = 0; i < ARRAY_COUNT(m); i++) {
        if (m[i].rm_so == -1) {
            break;
        }
        ptr_array_add(matches, xstrslice(buf, m[i].rm_so, m[i].rm_eo));
    }
    return true;
}
