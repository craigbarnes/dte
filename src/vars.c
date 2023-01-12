#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include "vars.h"
#include "buffer.h"
#include "editor.h"
#include "selection.h"
#include "util/array.h"
#include "util/bsearch.h"
#include "util/numtostr.h"
#include "util/path.h"
#include "util/xmalloc.h"
#include "view.h"

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

static char *expand_word(const EditorState *e)
{
    if (!e->view) {
        return NULL;
    }

    size_t size;
    char *selection = view_get_selection(e->view, &size);
    if (selection) {
        xrenew(selection, size + 1);
        selection[size] = '\0';
        return selection;
    }

    StringView word = view_get_word_under_cursor(e->view);
    return word.length ? xstrcut(word.data, word.length) : NULL;
}

static const BuiltinVar normal_vars[] = {
    {"COLNO", expand_colno},
    {"DTE_HOME", expand_dte_home},
    {"FILE", expand_file},
    {"FILEDIR", expand_file_dir},
    {"FILETYPE", expand_filetype},
    {"LINENO", expand_lineno},
    {"RFILE", expand_rfile},
    {"WORD", expand_word},
};

UNITTEST {
    CHECK_BSEARCH_ARRAY(normal_vars, name, strcmp);
}

bool expand_normal_var(const char *name, char **value, const void *userdata)
{
    const BuiltinVar *var = BSEARCH(name, normal_vars, vstrcmp);
    if (!var) {
        return false;
    }
    *value = var->expand(userdata);
    return true;
}

void collect_normal_vars(PointerArray *a, const char *prefix)
{
    COLLECT_STRING_FIELDS(normal_vars, name, a, prefix);
}
