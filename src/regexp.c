#include <errno.h>
#include <stdlib.h>
#include "regexp.h"
#include "util/arith.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/hashmap.h"
#include "util/intern.h"
#include "util/xmalloc.h"
#include "util/xstring.h"

// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
static HashMap interned_regexps = {.flags = HMAP_BORROWED_KEYS};

bool regexp_error_msg(ErrorBuffer *ebuf, const regex_t *re, const char *pattern, int err)
{
    if (!ebuf) {
        return false;
    }
    char msg[1024];
    regerror(err, re, msg, sizeof(msg));
    return error_msg(ebuf, "%s: %s", msg, pattern);
}

const regex_t *regexp_compile_or_fatal_error(const char *pattern)
{
    const InternedRegexp *ir = regexp_intern(NULL, pattern);
    FATAL_ERROR_ON(!ir, EINVAL);
    return &ir->re;
}

bool regexp_exec (
    const regex_t *re,
    const char *text,
    size_t text_len,
    size_t nmatch,
    regmatch_t *pmatch,
    int flags
) {
    BUG_ON(nmatch && !pmatch);

    if (HAVE_REG_STARTEND) {
        // "If REG_STARTEND is specified, pmatch must point to at least
        // one regmatch_t (even if nmatch is 0 or REG_NOSUB was specified),
        // to hold the input offsets for REG_STARTEND."
        // -- https://man.openbsd.org/regexec#:~:text=If-,REG_STARTEND,-is%20specified
        regmatch_t tmp_startend;
        pmatch = nmatch ? pmatch : &tmp_startend;
        pmatch[0].rm_so = 0;
        pmatch[0].rm_eo = text_len;
        return !regexec(re, text, nmatch, pmatch, flags | REGEXP_STARTEND_FLAG);
    }

    // Buffer must be null-terminated if REG_STARTEND isn't supported
    char *cstr = xstrcut(text, text_len);
    int ret = !regexec(re, cstr, nmatch, pmatch, flags);
    free(cstr);
    return ret;
}

// Check which word boundary tokens are supported by regcomp(3)
// (if any) and initialize `rwbt` with them for later use
RegexpWordBoundaryTokens regexp_get_word_boundary_tokens(void)
{
    static const char text[] = "SSfooEE SSfoo fooEE foo SSfooEE";
    const regoff_t match_start = 20, match_end = 23;
    static const RegexpWordBoundaryTokens pairs[] = {
        {"\\<", "\\>", 2},
        {"[[:<:]]", "[[:>:]]", 7},
        {"\\b", "\\b", 2},
    };

    BUG_ON(ARRAYLEN(text) <= match_end);
    BUG_ON(!mem_equal(text + match_start - 1, " foo ", 5));

    UNROLL_LOOP(ARRAYLEN(pairs))
    for (size_t i = 0; i < ARRAYLEN(pairs); i++) {
        const RegexpWordBoundaryTokens *p = &pairs[i];
        char patt[32];
        xmempcpy4(patt, p->start, p->len, STRN("(foo)"), p->end, p->len, "", 1);
        regex_t re;
        if (regcomp(&re, patt, DEFAULT_REGEX_FLAGS) != 0) {
            continue;
        }
        regmatch_t m[2];
        bool match = !regexec(&re, text, ARRAYLEN(m), m, 0);
        regfree(&re);
        if (match && m[0].rm_so == match_start && m[0].rm_eo == match_end) {
            return pairs[i];
        }
    }

    return (RegexpWordBoundaryTokens){.len = 0};
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

const InternedRegexp *regexp_intern(ErrorBuffer *ebuf, const char *pattern)
{
    if (pattern[0] == '\0') {
        return NULL;
    }

    InternedRegexp *ir = hashmap_get(&interned_regexps, pattern);
    if (ir) {
        return ir;
    }

    ir = xmalloc(sizeof(*ir));
    int err = regcomp(&ir->re, pattern, DEFAULT_REGEX_FLAGS | REG_NEWLINE | REG_NOSUB);
    if (unlikely(err)) {
        regexp_error_msg(ebuf, &ir->re, pattern, err);
        free(ir);
        return NULL;
    }

    BUG_ON(!(interned_regexps.flags & HMAP_BORROWED_KEYS));
    const char *str = str_intern(pattern);
    ir->str = str;
    return hashmap_insert(&interned_regexps, (char*)str, ir);
}

bool regexp_is_interned(const char *pattern)
{
    return !!hashmap_find(&interned_regexps, pattern);
}

static void free_interned_regexp(InternedRegexp *ir)
{
    regfree(&ir->re);
    free(ir);
}

void free_interned_regexps(void)
{
    BUG_ON(!(interned_regexps.flags & HMAP_BORROWED_KEYS));
    hashmap_free(&interned_regexps, FREE_FUNC(free_interned_regexp));
}
