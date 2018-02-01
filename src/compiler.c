#include "compiler.h"
#include "error.h"
#include "common.h"

static PointerArray compilers = PTR_ARRAY_INIT;

static Compiler *add_compiler(const char *name)
{
    Compiler *c = find_compiler(name);

    if (c) {
        return c;
    }

    c = xnew0(Compiler, 1);
    c->name = xstrdup(name);
    ptr_array_add(&compilers, c);
    return c;
}

Compiler *find_compiler(const char *name)
{
    for (size_t i = 0; i < compilers.count; i++) {
        Compiler *c = compilers.ptrs[i];
        if (streq(c->name, name)) {
            return c;
        }
    }
    return NULL;
}

void add_error_fmt (
    const char *compiler,
    bool ignore,
    const char *format,
    char **desc
) {
    const char *const names[] = {"file", "line", "column", "message"};
    int idx[ARRAY_COUNT(names)] = {-1, -1, -1, 0};
    ErrorFormat *f;

    for (size_t i = 0, j = 0; desc[i]; i++) {
        for (j = 0; j < ARRAY_COUNT(names); j++) {
            if (streq(desc[i], names[j])) {
                idx[j] = ((int)i) + 1;
                break;
            }
        }
        if (j == ARRAY_COUNT(names)) {
            error_msg("Unknown substring name %s.", desc[i]);
            return;
        }
    }

    f = xnew0(ErrorFormat, 1);
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
        if (idx[i] > (int)f->re.re_nsub) {
            error_msg("Invalid substring count.");
            regfree(&f->re);
            free(f);
            return;
        }
    }

    ptr_array_add(&add_compiler(compiler)->error_formats, f);
}
