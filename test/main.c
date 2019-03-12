#include <langinfo.h>
#include <locale.h>
#include <string.h>
#include "test.h"
#include "../src/command.h"
#include "../src/editor.h"
#include "../src/util/path.h"
#include "../src/util/xmalloc.h"

void test_encoding(void);
void test_filetype(void);
void test_terminal(void);
void test_util(void);
void init_headless_mode(void);
void test_exec_config(void);

static void test_relative_filename(void)
{
    static const struct rel_test {
        const char *cwd, *path, *result;
    } tests[] = { // NOTE: at most 2 ".." components allowed in relative name
        { "/", "/", "/" },
        { "/", "/file", "file" },
        { "/a/b/c/d", "/a/b/file", "../../file" },
        { "/a/b/c/d/e", "/a/b/file", "/a/b/file" },
        { "/a/foobar", "/a/foo/file", "../foo/file" },
    };
    FOR_EACH_I(i, tests) {
        const struct rel_test *t = &tests[i];
        char *result = relative_filename(t->path, t->cwd);
        IEXPECT_STREQ(t->result, result);
        free(result);
    }
}

// Checks that `commands` array is sorted in binary searchable order
static void test_commands_sort(void)
{
    size_t n = 0;
    while (commands[n].name) {
        n++;
        BUG_ON(n > 500);
    }
    for (size_t i = 1; i < n; i++) {
        IEXPECT_GT(strcmp(commands[i].name, commands[i - 1].name), 0);
    }
}

int main(void)
{
    init_editor_state();

    test_relative_filename();
    test_commands_sort();

    test_encoding();
    test_filetype();
    test_util();
    test_terminal();

    init_headless_mode();
    test_exec_config();

    return failed ? 1 : 0;
}
