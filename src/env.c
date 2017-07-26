#include "env.h"
#include "completion.h"
#include "window.h"
#include "selection.h"
#include "editor.h"

struct builtin_env {
	const char *name;
	char *(*expand)(void);
};

static char *expand_file(void)
{
	struct view *v = window->view;

	if (v->buffer->abs_filename == NULL) {
		return xstrdup("");
	}
	return xstrdup(v->buffer->abs_filename);
}

static char *expand_pkgdatadir(void)
{
	return xstrdup(pkgdatadir);
}

static char *expand_word(void)
{
	struct view *v = window->view;
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

static const struct builtin_env builtin[] = {
	{ "FILE",	expand_file },
	{ "PKGDATADIR",	expand_pkgdatadir },
	{ "WORD",	expand_word },
};

void collect_builtin_env(const char *prefix)
{
	int i;

	for (i = 0; i < ARRAY_COUNT(builtin); i++) {
		const char *name = builtin[i].name;
		if (str_has_prefix(name, prefix))
			add_completion(xstrdup(name));
	}
}

// returns NULL only if name isn't in builtin array
char *expand_builtin_env(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_COUNT(builtin); i++) {
		const struct builtin_env *be = &builtin[i];
		if (streq(be->name, name)) {
			return be->expand();
		}
	}
	return NULL;
}
