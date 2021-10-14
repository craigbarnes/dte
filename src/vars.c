#include <stddef.h>
#include "vars.h"
#include "buffer.h"
#include "completion.h"
#include "editor.h"
#include "error.h"
#include "selection.h"
#include "util/numtostr.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "view.h"

typedef struct {
    const char *name;
    char *(*expand)(void);
} BuiltinVar;

static char *expand_dte_home(void)
{
    return xstrdup(editor.user_config_dir);
}

static char *expand_file(void)
{
    if (!buffer || !buffer->abs_filename) {
        return NULL;
    }
    return xstrdup(buffer->abs_filename);
}

static char *expand_filetype(void)
{
    return buffer ? xstrdup(buffer->options.filetype) : NULL;
}

static char *expand_lineno(void)
{
    return view ? xstrdup(umax_to_str(view->cy + 1)) : NULL;
}

static char *expand_word(void)
{
    if (!view) {
        return NULL;
    }

    size_t size;
    char *selection = view_get_selection(view, &size);
    if (selection) {
        xrenew(selection, size + 1);
        selection[size] = '\0';
        return selection;
    }

    StringView word = view_get_word_under_cursor(view);
    return word.length ? xstrcut(word.data, word.length) : NULL;
}

static char *expand_pkgdatadir(void)
{
    error_msg("The $PKGDATADIR variable was removed in dte v1.4");
    return NULL;
}

static const BuiltinVar normal_vars[] = {
    {"PKGDATADIR", expand_pkgdatadir},
    {"DTE_HOME", expand_dte_home},
    {"FILE", expand_file},
    {"FILETYPE", expand_filetype},
    {"LINENO", expand_lineno},
    {"WORD", expand_word},
};

bool expand_normal_var(const char *name, char **value)
{
    for (size_t i = 0; i < ARRAY_COUNT(normal_vars); i++) {
        if (streq(name, normal_vars[i].name)) {
            *value = normal_vars[i].expand();
            return true;
        }
    }
    return false;
}

bool expand_syntax_var(const char *name, char **value)
{
    if (streq(name, "DTE_HOME")) {
        *value = expand_dte_home();
        return true;
    }
    return false;
}

void collect_normal_vars(PointerArray *a, const char *prefix)
{
    for (size_t i = 0; i < ARRAY_COUNT(normal_vars); i++) {
        const char *name = normal_vars[i].name;
        if (str_has_prefix(name, prefix)) {
            ptr_array_append(a, xstrdup(name));
        }
    }
}
