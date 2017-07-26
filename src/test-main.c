#include "editor.h"
#include "common.h"
#include "path.h"

#include <locale.h>
#include <langinfo.h>

static void fail(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

static void test_relative_filename(void)
{
	static const struct rel_test {
		const char *cwd;
		const char *path;
		const char *result;
	} tests[] = {
		// NOTE: at most 2 ".." components allowed in relative name
		{ "/", "/", "/" },
		{ "/", "/file", "file" },
		{ "/a/b/c/d", "/a/b/file", "../../file" },
		{ "/a/b/c/d/e", "/a/b/file", "/a/b/file" }, // "../../../file" contains too many ".." components
		{ "/a/foobar", "/a/foo/file", "../foo/file" },
	};
	int i;

	for (i = 0; i < ARRAY_COUNT(tests); i++) {
		const struct rel_test *t = &tests[i];
		char *result = relative_filename(t->path, t->cwd);
		if (!streq(t->result, result))
			fail("relative_filename(%s, %s) -> %s, expected %s\n", t->path, t->cwd, result, t->result);
		free(result);
	}
}

int main(int argc, char *argv[])
{
	const char *home = getenv("HOME");

	if (!home)
		home = "";
	home_dir = xstrdup(home);

	setlocale(LC_CTYPE, "");
	charset = nl_langinfo(CODESET);
	if (streq(charset, "UTF-8"))
		term_utf8 = true;

	test_relative_filename();
	return 0;
}
