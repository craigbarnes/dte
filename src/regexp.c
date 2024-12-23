#include <errno.h>
#include <stdlib.h>
#include "regexp.h"
#include "error.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/hashmap.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "util/xstring.h"

// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
static HashMap interned_regexps;

bool regexp_error_msg(const regex_t *re, const char *pattern, int err)
{
    char msg[1024];
    regerror(err, re, msg, sizeof(msg));
    return error_msg("%s: %s", msg, pattern);
}

bool regexp_compile_internal(regex_t *re, const char *pattern, int flags)
{
    int err = regcomp(re, pattern, flags);
    return !err || regexp_error_msg(re, pattern, err);
}

void regexp_compile_or_fatal_error(regex_t *re, const char *pattern, int flags)
{
    // Note: DEFAULT_REGEX_FLAGS isn't used here because this function
    // is only used for compiling built-in patterns, where we explicitly
    // avoid using "enhanced" features
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
#if defined(REG_STARTEND) && ASAN_ENABLED == 0 && MSAN_ENABLED == 0
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

// Check which word boundary tokens are supported by regcomp(3)
// (if any) and initialize `rwbt` with them for later use
bool regexp_init_word_boundary_tokens(RegexpWordBoundaryTokens *rwbt)
{
    static const char text[] = "SSfooEE SSfoo fooEE foo SSfooEE";
    const regoff_t match_start = 20, match_end = 23;
    static const RegexpWordBoundaryTokens pairs[] = {
        {"\\<", "\\>"},
        {"[[:<:]]", "[[:>:]]"},
        {"\\b", "\\b"},
    };

    BUG_ON(ARRAYLEN(text) <= match_end);
    BUG_ON(!mem_equal(text + match_start - 1, " foo ", 5));

    for (size_t i = 0; i < ARRAYLEN(pairs); i++) {
        const char *start = pairs[i].start;
        const char *end = pairs[i].end;
        char patt[32];
        xsnprintf(patt, sizeof(patt), "%s(foo)%s", start, end);
        regex_t re;
        if (regcomp(&re, patt, DEFAULT_REGEX_FLAGS) != 0) {
            continue;
        }
        regmatch_t m[2];
        bool match = !regexec(&re, text, ARRAYLEN(m), m, 0);
        regfree(&re);
        if (match && m[0].rm_so == match_start && m[0].rm_eo == match_end) {
            *rwbt = pairs[i];
            return true;
        }
    }

    return false;
}

size_t regexp_escapeb(char *buf, size_t buflen, const char *pat, size_t plen)
{
    BUG_ON(buflen < (2 * plen) + 1);
    size_t o = 0;
    for (size_t i = 0; i < plen; i++) {
        char ch = pat[i];
        if (is_regex_special_char(ch)) {
            buf[o++] = '\\';
        }
        buf[o++] = ch;
    }
    buf[o] = '\0';
    return o;
}

char *regexp_escape(const char *pattern, size_t len)
{
    size_t buflen = xmul(2, len) + 1;
    char *buf = xmalloc(buflen);
    regexp_escapeb(buf, buflen, pattern, len);
    return buf;
}

const InternedRegexp *regexp_intern(const char *pattern)
{
    if (pattern[0] == '\0') {
        return NULL;
    }

    InternedRegexp *ir = hashmap_get(&interned_regexps, pattern);
    if (ir) {
        return ir;
    }

    ir = xnew(InternedRegexp, 1);
    int err = regcomp(&ir->re, pattern, DEFAULT_REGEX_FLAGS | REG_NEWLINE | REG_NOSUB);
    if (unlikely(err)) {
        regexp_error_msg(&ir->re, pattern, err);
        free(ir);
        return NULL;
    }

    char *str = xstrdup(pattern);
    ir->str = str;
    return hashmap_insert(&interned_regexps, str, ir);
}

bool regexp_is_interned(const char *pattern)
{
    return !!hashmap_find(&interned_regexps, pattern);
}

// Note: this does NOT free InternedRegexp::str, because it points at the
// same string as HashMapEntry::key and is already freed by hashmap_free()
static void free_interned_regexp(InternedRegexp *ir)
{
    regfree(&ir->re);
    free(ir);
}

void free_interned_regexps(void)
{
    hashmap_free(&interned_regexps, (FreeFunction)free_interned_regexp);
}
