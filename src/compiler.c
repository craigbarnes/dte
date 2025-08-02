#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "command/serialize.h"
#include "util/array.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

static const char capture_names[NR_ERRFMT_INDICES][8] = {
    [ERRFMT_FILE] = "file",
    [ERRFMT_LINE] = "line",
    [ERRFMT_COLUMN] = "column",
    [ERRFMT_MESSAGE] = "message"
};

UNITTEST {
    CHECK_STRING_ARRAY(capture_names);
}

ssize_t errorfmt_capture_name_to_index(const char *name)
{
    for (size_t i = 0; i < ARRAYLEN(capture_names); i++) {
        if (streq(name, capture_names[i])) {
            return i;
        }
    }
    return -1;
}

static Compiler *find_or_add_compiler(HashMap *compilers, const char *name)
{
    Compiler *c = find_compiler(compilers, name);
    return c ? c : hashmap_insert(compilers, xstrdup(name), xcalloc1(sizeof(*c)));
}

void add_error_fmt (
    HashMap *compilers,
    const char *name,
    const char *pattern,
    regex_t *re,
    int8_t idx[static NR_ERRFMT_INDICES],
    bool ignore
) {
    ErrorFormat *f = xmalloc(sizeof(*f));
    f->ignore = ignore;
    f->re = *re; // Takes ownership (responsible for calling regfree(3))
    memcpy(f->capture_index, idx, NR_ERRFMT_INDICES);

    Compiler *compiler = find_or_add_compiler(compilers, name);
    f->pattern = str_intern(pattern);
    ptr_array_append(&compiler->error_formats, f);
}

void free_error_format(ErrorFormat *f)
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
    static const char underscore[][2] = {"_"};
    COLLECT_STRINGS(underscore, a, prefix);
    COLLECT_STRINGS(capture_names, a, prefix);
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
