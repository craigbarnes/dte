#include <stdlib.h>
#include "compiler.h"
#include "command/serialize.h"
#include "completion.h"
#include "error.h"
#include "regexp.h"
#include "util/hashmap.h"
#include "util/hashset.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

static const char capture_names[][8] = {
    [ERRFMT_FILE] = "file",
    [ERRFMT_LINE] = "line",
    [ERRFMT_COLUMN] = "column",
    [ERRFMT_MESSAGE] = "message"
};

static HashMap compilers = HASHMAP_INIT;

static Compiler *find_or_add_compiler(const char *name)
{
    Compiler *c = find_compiler(name);
    if (c) {
        return c;
    }

    c = xnew0(Compiler, 1);
    hashmap_insert(&compilers, xstrdup(name), c);
    return c;
}

Compiler *find_compiler(const char *name)
{
    return hashmap_get(&compilers, name);
}

void add_error_fmt (
    const char *compiler,
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
    for (size_t i = 0, j = 0, n = ARRAY_COUNT(capture_names); desc[i]; i++) {
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
            error_msg("unknown substring name %s", desc[i]);
            return;
        }
    }

    ErrorFormat *f = xnew(ErrorFormat, 1);
    f->ignore = ignore;
    static_assert_compatible_types(f->capture_index, idx);
    memcpy(f->capture_index, idx, sizeof(idx));

    if (unlikely(!regexp_compile(&f->re, format, 0))) {
        free(f);
        return;
    }

    if (unlikely(max_idx > f->re.re_nsub)) {
        error_msg("invalid substring count");
        regfree(&f->re);
        free(f);
        return;
    }

    f->pattern = str_intern(format);
    ptr_array_append(&find_or_add_compiler(compiler)->error_formats, f);
}

void collect_compilers(PointerArray *a, const char *prefix)
{
    collect_hashmap_keys(&compilers, a, prefix);
}

void collect_errorfmt_capture_names(PointerArray *a, const char *prefix)
{
    for (size_t i = 0; i < ARRAY_COUNT(capture_names); i++) {
        if (str_has_prefix(capture_names[i], prefix)) {
            ptr_array_append(a, xstrdup(capture_names[i]));
        }
    }
    if (str_has_prefix("_", prefix)) {
        ptr_array_append(a, xstrdup("_"));
    }
}

static void append_compiler(String *s, const Compiler *c, const char *name)
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

        static_assert(ARRAY_COUNT(e->capture_index) == 4);
        const int8_t *a = e->capture_index;
        int max_idx = MAX4(a[0], a[1], a[2], a[3]);
        BUG_ON(max_idx > ERRORFMT_CAPTURE_MAX);

        for (int j = 1; j <= max_idx; j++) {
            const char *capname = "_";
            for (size_t k = 0; k < ARRAY_COUNT(capture_names); k++) {
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

String dump_compiler(const Compiler *c, const char *name)
{
    String buf = string_new(512);
    append_compiler(&buf, c, name);
    return buf;
}

String dump_compilers(void)
{
    String buf = string_new(4096);
    for (HashMapIter it = hashmap_iter(&compilers); hashmap_next(&it); ) {
        const char *name = it.entry->key;
        const Compiler *c = it.entry->value;
        append_compiler(&buf, c, name);
        string_append_byte(&buf, '\n');
    }
    return buf;
}
