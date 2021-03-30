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

static HashMap compilers = HASHMAP_INIT;

static Compiler *add_compiler(const char *name)
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
    static const char names[][8] = {"file", "line", "column", "message"};
    int idx[ARRAY_COUNT(names)] = {-1, -1, -1, 0};

    for (size_t i = 0, j = 0; desc[i]; i++) {
        for (j = 0; j < ARRAY_COUNT(names); j++) {
            if (streq(desc[i], names[j])) {
                idx[j] = ((int)i) + 1;
                break;
            }
        }
        if (streq(desc[i], "_")) {
            continue;
        }
        if (unlikely(j == ARRAY_COUNT(names))) {
            error_msg("Unknown substring name %s.", desc[i]);
            return;
        }
    }

    ErrorFormat *f = xnew(ErrorFormat, 1);
    f->ignore = ignore;
    f->msg_idx = idx[3];
    f->file_idx = idx[0];
    f->line_idx = idx[1];
    f->column_idx = idx[2];

    if (!regexp_compile(&f->re, format, 0)) {
        free(f);
        return;
    }

    for (size_t i = 0; i < ARRAY_COUNT(idx); i++) {
        // NOTE: -1 is larger than 0UL
        if (unlikely(idx[i] > (int)f->re.re_nsub)) {
            error_msg("Invalid substring count.");
            regfree(&f->re);
            free(f);
            return;
        }
    }

    f->pattern = str_intern(format);
    ptr_array_append(&add_compiler(compiler)->error_formats, f);
}

void collect_compilers(const char *prefix)
{
    collect_hashmap_keys(&compilers, prefix);
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

        int max_idx = MAX4(e->file_idx, e->line_idx, e->column_idx, e->msg_idx);
        BUG_ON(max_idx > 16);

        for (int j = 1; j <= max_idx; j++) {
            const char *idx_type = "_";
            if (j == e->file_idx) {
                idx_type = "file";
            } else if (j == e->line_idx) {
                idx_type = "line";
            } else if (j == e->column_idx) {
                idx_type = "column";
            } else if (j == e->msg_idx) {
                idx_type = "message";
            }
            string_append_byte(s, ' ');
            string_append_cstring(s, idx_type);
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
