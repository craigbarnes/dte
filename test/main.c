#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <string.h>
#include "test.h"
#include "../src/command.h"
#include "../src/debug.h"
#include "../src/editor.h"
#include "../src/util/path.h"
#include "../src/util/xmalloc.h"

void test_cmdline(void);
void test_editorconfig(void);
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

static void test_commands_array(void)
{
    static const size_t name_size = ARRAY_COUNT(commands[0].name);
    static const size_t flags_size = ARRAY_COUNT(commands[0].flags);

    size_t n = 0;
    while (commands[n].cmd) {
        n++;
        BUG_ON(n > 500);
    }

    for (size_t i = 1; i < n; i++) {
        const Command *const cmd = &commands[i];

        // Check that fixed-size arrays are null-terminated within bounds
        ASSERT_EQ(cmd->name[name_size - 1], '\0');
        ASSERT_EQ(cmd->flags[flags_size - 1], '\0');

        // Check that array is sorted by name field, in binary searchable order
        IEXPECT_GT(strcmp(cmd->name, commands[i - 1].name), 0);
    }
}

static void test_posix_sanity(void)
{
    // These assertions are not guaranteed by ISO C99, but they are required
    // by POSIX and are relied upon by this codebase.
    ASSERT_TRUE(NULL == (void*)0);
    ASSERT_EQ(CHAR_BIT, 8);
}

int main(void)
{
    test_posix_sanity();
    init_editor_state();

    test_relative_filename();
    test_commands_array();

    test_editorconfig();
    test_encoding();
    test_filetype();
    test_util();
    test_terminal();
    test_cmdline();

    init_headless_mode();
    test_exec_config();

    return failed ? 1 : 0;
}
