#include "env.h"
#include "completion.h"
#include "window.h"
#include "selection.h"
#include "editor.h"

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
    View *v = window->view;

    if (v->buffer->abs_filename == NULL) {
        return xstrdup("");
    }
    return xstrdup(v->buffer->abs_filename);
}

static char *expand_word(void)
{
    View *v = window->view;
    long size;
    char *str = view_get_selection(v, &size);

    if (str != NULL) {
        xrenew(str, size + 1);
        str[size] = 0;
    } else {
        str = view_get_word_under_cursor(v);
        if (str == NULL) {
            str = xstrdup("");
        }
    }
    return str;
}

static char *expand_pkgdatadir(void)
{
    static bool warned;
    if (!warned) {
        fputs (
            "\n\033[1;31mNOTICE:\033[0m "
            "$PKGDATADIR has been removed from dte\n\n"
            "  The command \"include $PKGDATADIR/...\" is now"
            " \"include -b ...\"\n\n"
            "  See: https://github.com/craigbarnes/dte/issues/70\n\n\n",
            stderr
        );
        editor.everything_changed = true;
        warned = true;
    }
    return xstrdup("");
}

static const BuiltinEnv builtin[] = {
    {"PKGDATADIR", expand_pkgdatadir},
    {"DTE_HOME", expand_dte_home},
    {"FILE", expand_file},
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
char *expand_builtin_env(const char *name)
{
    for (size_t i = 0; i < ARRAY_COUNT(builtin); i++) {
        const BuiltinEnv *be = &builtin[i];
        if (streq(be->name, name)) {
            return be->expand();
        }
    }
    return NULL;
}
