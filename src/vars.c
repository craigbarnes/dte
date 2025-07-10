#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include "vars.h"
#include "editor.h"
#include "selection.h"
#include "util/array.h"
#include "util/bsearch.h"
#include "util/numtostr.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "view.h"

// TODO: Make expand() functions return a StringVariant type and
// only do dynamic allocations when necessary
typedef struct {
    char name[12];
    char *(*expand)(const EditorState *e);
} BuiltinVar;

static char *expand_dte_home(const EditorState *e)
{
    return xstrdup(e->user_config_dir);
}

static char *expand_file(const EditorState *e)
{
    if (!e->buffer || !e->buffer->abs_filename) {
        return NULL;
    }
    return xstrdup(e->buffer->abs_filename);
}

static char *expand_file_dir(const EditorState *e)
{
    if (!e->buffer || !e->buffer->abs_filename) {
        return NULL;
    }
    return path_dirname(e->buffer->abs_filename);
}

static char *expand_rfile(const EditorState *e)
{
    if (!e->buffer || !e->buffer->abs_filename) {
        return NULL;
    }
    char buf[8192];
    const char *cwd = getcwd(buf, sizeof buf);
    const char *abs = e->buffer->abs_filename;
    return likely(cwd) ? path_relative(abs, cwd) : xstrdup(abs);
}

static char *expand_rfiledir(const EditorState *e)
{
    char *rfile = expand_rfile(e);
    if (!rfile) {
        return NULL;
    }

    StringView dir = path_slice_dirname(rfile);
    if (rfile != (char*)dir.data) {
        // rfile contains no directory part (i.e. no slashes), so there's
        // nothing to "slice" and we simply return "." instead
        free(rfile);
        return xstrdup(".");
    }

    // Overwrite the last slash in rfile with a null terminator, so as
    // to remove the last path component (the filename)
    rfile[dir.length] = '\0';
    return rfile;
}

static char *expand_filetype(const EditorState *e)
{
    return e->buffer ? xstrdup(e->buffer->options.filetype) : NULL;
}

static char *expand_colno(const EditorState *e)
{
    return e->view ? xstrdup(umax_to_str(e->view->cx_display + 1)) : NULL;
}

static char *expand_lineno(const EditorState *e)
{
    return e->view ? xstrdup(umax_to_str(e->view->cy + 1)) : NULL;
}

static char *msgpos(const EditorState *e, size_t idx)
{
    return xstrdup(umax_to_str(e->messages[idx].pos + 1));
}

static char *expand_msgpos_a(const EditorState *e) {return msgpos(e, 0);}
static char *expand_msgpos_b(const EditorState *e) {return msgpos(e, 1);}
static char *expand_msgpos_c(const EditorState *e) {return msgpos(e, 2);}

static char *expand_word(const EditorState *e)
{
    const View *view = e->view;
    if (unlikely(!view)) {
        return NULL;
    }

    if (view->selection) {
        SelectionInfo info = init_selection(view);
        size_t len = info.eo - info.so;
        if (unlikely(len == 0)) {
            return NULL;
        }
        char *selection = block_iter_get_bytes(&info.si, len);
        BUG_ON(!selection);
        selection[len] = '\0'; // See comment in block_iter_get_bytes()
        return selection;
    }

    StringView word = view_get_word_under_cursor(view);
    return word.length ? xstrcut(word.data, word.length) : NULL;
}

static const BuiltinVar normal_vars[] = {
    {"COLNO", expand_colno},
    {"DTE_HOME", expand_dte_home},
    {"FILE", expand_file},
    {"FILEDIR", expand_file_dir},
    {"FILETYPE", expand_filetype},
    {"LINENO", expand_lineno},
    {"MSGPOS", expand_msgpos_a},
    {"MSGPOS_A", expand_msgpos_a},
    {"MSGPOS_B", expand_msgpos_b},
    {"MSGPOS_C", expand_msgpos_c},
    {"RFILE", expand_rfile},
    {"RFILEDIR", expand_rfiledir},
    {"WORD", expand_word},
};

UNITTEST {
    CHECK_BSEARCH_ARRAY(normal_vars, name);
}

// Handles variables like e.g. ${builtin:color/reset} and ${script:file.sh}
static char *expand_prefixed_var(const char *name, size_t name_len, StringView prefix)
{
    char buf[64];
    bool name_fits_buf = (name_len < sizeof(buf));

    if (strview_equal_cstring(&prefix, "builtin")) {
        // Skip past "builtin:"
        name += prefix.length + 1;
    } else if (strview_equal_cstring(&prefix, "script") && name_fits_buf) {
        // Copy `name` into `buf` and replace ':' with '/'
        memcpy(buf, name, name_len + 1);
        buf[prefix.length] = '/';
        name = buf;
    } else {
        return NULL;
    }

    const BuiltinConfig *cfg = get_builtin_config(name);
    return cfg ? xmemdup(cfg->text.data, cfg->text.length + 1) : NULL;
}

char *expand_normal_var(const EditorState *e, const char *name)
{
    size_t name_len = strlen(name);
    if (unlikely(name_len == 0)) {
        return NULL;
    }

    size_t pos = 0;
    StringView prefix = get_delim(name, &pos, name_len, ':');
    if (prefix.length < name_len) {
        return expand_prefixed_var(name, name_len, prefix);
    }

    const BuiltinVar *var = BSEARCH(name, normal_vars, vstrcmp);
    if (var) {
        return var->expand(e);
    }

    const char *str = xgetenv(name);
    return str ? xstrdup(str) : NULL;
}

void collect_normal_vars (
    PointerArray *a,
    StringView prefix, // Prefix to match against
    const char *suffix // Suffix to append to collected strings
) {
    size_t suffix_len = strlen(suffix) + 1;
    for (size_t i = 0; i < ARRAYLEN(normal_vars); i++) {
        const char *var = normal_vars[i].name;
        size_t var_len = strlen(var);
        if (strn_has_strview_prefix(var, var_len, &prefix)) {
            ptr_array_append(a, xmemjoin(var, var_len, suffix, suffix_len));
        }
    }
}
