#include "env.h"
#include "completion.h"
#include "debug.h"
#include "editor.h"
#include "error.h"
#include "selection.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "window.h"

typedef struct {
    const char *name;
    char *(*expand)(void);
} BuiltinEnv;

static char *expand_dte_home(void)
{
    return xstrdup(editor.user_config_dir);
}

static char *expand_file(void)
{
    if (editor.status != EDITOR_RUNNING) {
        return NULL;
    }
    const char *filename = buffer->abs_filename;
    return filename ? xstrdup(filename) : NULL;
}

static char *expand_filetype(void)
{
    if (editor.status != EDITOR_RUNNING) {
        return NULL;
    }
    return xstrdup(buffer->options.filetype);
}

static char *expand_lineno(void)
{
    if (editor.status != EDITOR_RUNNING) {
        return NULL;
    }
    return xasprintf("%ld", view->cy + 1);
}

static char *expand_word(void)
{
    if (editor.status != EDITOR_RUNNING) {
        return NULL;
    }
    size_t size;
    char *str = view_get_selection(view, &size);
    if (str != NULL) {
        xrenew(str, size + 1);
        str[size] = '\0';
    } else {
        str = view_get_word_under_cursor(view);
        if (str == NULL) {
            str = NULL;
        }
    }
    return str;
}

static char *expand_pkgdatadir(void)
{
    error_msg("The $PKGDATADIR variable was removed in dte v1.4");
    return NULL;
}

static const BuiltinEnv builtin[] = {
    {"PKGDATADIR", expand_pkgdatadir},
    {"DTE_HOME", expand_dte_home},
    {"FILE", expand_file},
    {"FILETYPE", expand_filetype},
    {"LINENO", expand_lineno},
    {"WORD", expand_word},
};

void collect_builtin_env(const char *prefix)
{
    for (size_t i = 0; i < ARRAY_COUNT(builtin); i++) {
        const char *name = builtin[i].name;
        if (str_has_prefix(name, prefix)) {
            add_completion(xstrdup(name));
        }
    }
}

// Returns NULL only if name isn't in builtin array
bool expand_builtin_env(const char *name, char **value)
{
    for (size_t i = 0; i < ARRAY_COUNT(builtin); i++) {
        const BuiltinEnv *be = &builtin[i];
        if (streq(be->name, name)) {
            *value = be->expand();
            return true;
        }
    }
    return false;
}
