#include "regexp.h"
#include "error.h"
#include "common.h"

bool regexp_match_nosub(const char *pattern, const char *buf, long size)
{
    regmatch_t m;
    regex_t re;
    bool ret;

    BUG_ON(!regexp_compile(&re, pattern, REG_NEWLINE | REG_NOSUB));
    ret = regexp_exec(&re, buf, size, 1, &m, 0);
    regfree(&re);
    return ret;
}

bool regexp_match (
    const char *pattern,
    const char *buf,
    long size,
    PointerArray *m
) {
    regex_t re;
    bool ret;

    BUG_ON(!regexp_compile(&re, pattern, REG_NEWLINE));
    ret = regexp_exec_sub(&re, buf, size, m, 0);
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
    long size,
    long nr_m,
    regmatch_t *m,
    int flags
) {
#ifdef REG_STARTEND
    BUG_ON(!nr_m);
    m[0].rm_so = 0;
    m[0].rm_eo = size;
    return !regexec(re, buf, nr_m, m, flags | REG_STARTEND);
#else
    // Buffer must be null-terminated string if REG_STARTED is not supported
    char *tmp = xnew(char, size + 1);
    int ret;

    BUG_ON(!nr_m);
    memcpy(tmp, buf, size);
    tmp[size] = 0;
    ret = !regexec(re, tmp, nr_m, m, flags);
    free(tmp);
    return ret;
#endif
}

bool regexp_exec_sub (
    const regex_t *re,
    const char *buf,
    long size,
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
