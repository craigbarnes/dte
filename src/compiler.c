#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "command/serialize.h"
#include "error.h"
#include "regexp.h"
#include "util/array.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

static const char capture_names[][8] = {
    [ERRFMT_FILE] = "file",
    [ERRFMT_LINE] = "line",
    [ERRFMT_COLUMN] = "column",
    [ERRFMT_MESSAGE] = "message"
};

UNITTEST {
    CHECK_STRING_ARRAY(capture_names);
}

static Compiler *find_or_add_compiler(HashMap *compilers, const char *name)
{
    Compiler *c = find_compiler(compilers, name);
    return c ? c : hashmap_insert(compilers, xstrdup(name), xnew0(Compiler, 1));
}

Compiler *find_compiler(const HashMap *compilers, const char *name)
{
    return hashmap_get(compilers, name);
}

bool add_error_fmt (
    HashMap *compilers,
    const char *name,
    bool ignore,
    const char *format,
    char **desc
) {
    int8_t idx[] = {
        [ERRFMT_FILE] = -1,
        [ERRFMT_LINE] = -1,
        [ERRFMT_COLUMN] = -1,
        [ERRFMT_MESSAGE] = 0,
    };

    size_t max_idx = 0;
    for (size_t i = 0, j = 0, n = ARRAYLEN(capture_names); desc[i]; i++) {
        BUG_ON(i >= ERRORFMT_CAPTURE_MAX);
        if (streq(desc[i], "_")) {
            continue;
        }
        for (j = 0; j < n; j++) {
            if (streq(desc[i], capture_names[j])) {
                max_idx = i + 1;
                idx[j] = max_idx;
                break;
            }
        }
        if (unlikely(j == n)) {
            return error_msg("unknown substring name %s", desc[i]);
        }
    }

    ErrorFormat *f = xnew(ErrorFormat, 1);
    f->ignore = ignore;
    static_assert_compatible_types(f->capture_index, idx);
    memcpy(f->capture_index, idx, sizeof(idx));

    if (unlikely(!regexp_compile(&f->re, format, 0))) {
        free(f);
        return false;
    }

    if (unlikely(max_idx > f->re.re_nsub)) {
        regfree(&f->re);
        free(f);
        return error_msg("invalid substring count");
    }

    Compiler *compiler = find_or_add_compiler(compilers, name);
    f->pattern = str_intern(format);
    ptr_array_append(&compiler->error_formats, f);
    return true;
}

static void free_error_format(ErrorFormat *f)
{
    regfree(&f->re);
    free(f);
}

void free_compiler(Compiler *c)
{
    ptr_array_free_cb(&c->error_formats, FREE_FUNC(free_error_format));
    free(c);
}

void remove_compiler(HashMap *compilers, const char *name)
{
    Compiler *c = hashmap_remove(compilers, name);
    if (c) {
        free_compiler(c);
    }
}

void collect_errorfmt_capture_names(PointerArray *a, const char *prefix)
{
    COLLECT_STRINGS(capture_names, a, prefix);
    if (str_has_prefix("_", prefix)) {
        ptr_array_append(a, xstrdup("_"));
    }
}

void dump_compiler(const Compiler *c, const char *name, String *s)
{
    for (size_t i = 0, n = c->error_formats.count; i < n; i++) {
        ErrorFormat *e = c->error_formats.ptrs[i];
        string_append_literal(s, "errorfmt ");
        if (e->ignore) {
            string_append_literal(s, "-i ");
        }
        if (unlikely(name[0] == '-' || e->pattern[0] == '-')) {
            string_append_literal(s, "-- ");
        }
        string_append_escaped_arg(s, name, true);
        string_append_byte(s, ' ');
        string_append_escaped_arg(s, e->pattern, true);

        static_assert(ARRAYLEN(e->capture_index) == 4);
        const int8_t *a = e->capture_index;
        int max_idx = MAX4(a[0], a[1], a[2], a[3]);
        BUG_ON(max_idx > ERRORFMT_CAPTURE_MAX);

        for (int j = 1; j <= max_idx; j++) {
            const char *capname = "_";
            for (size_t k = 0; k < ARRAYLEN(capture_names); k++) {
                if (j == a[k]) {
                    capname = capture_names[k];
                    break;
                }
            }
            string_append_byte(s, ' ');
            string_append_cstring(s, capname);
        }

        string_append_byte(s, '\n');
    }
}
