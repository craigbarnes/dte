#include <stddef.h>
#include "env.h"
#include "buffer.h"
#include "completion.h"
#include "editor.h"
#include "error.h"
#include "selection.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "view.h"

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
    if (!view) {
        return NULL;
    }
    size_t maxlen = DECIMAL_STR_MAX(view->cy);
    char *str = xmalloc(maxlen);
    xsnprintf(str, maxlen, "%ld", view->cy + 1);
    return str;
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
