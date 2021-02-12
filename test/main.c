#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "test.h"
#include "bind.h"
#include "block.h"
#include "buffer.h"
#include "commands.h"
#include "editor.h"
#include "regexp.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

void init_headless_mode(void);
extern const TestGroup cmdline_tests;
extern const TestGroup command_tests;
extern const TestGroup config_tests;
extern const TestGroup editorconfig_tests;
extern const TestGroup encoding_tests;
extern const TestGroup filetype_tests;
extern const TestGroup history_tests;
extern const TestGroup option_tests;
extern const TestGroup syntax_tests;
extern const TestGroup terminal_tests;
extern const TestGroup util_tests;

static void test_handle_binding(void)
{
    handle_command(&commands, "bind ^A 'insert zzz'; open", false);

    // Bound command should be cached
    const KeyBinding *binding = lookup_binding(MOD_CTRL | 'A');
    const Command *insert = find_normal_command("insert");
    ASSERT_NONNULL(binding);
    ASSERT_NONNULL(insert);
    EXPECT_PTREQ(binding->cmd, insert);
    EXPECT_EQ(binding->a.nr_flags, 0);
    EXPECT_EQ(binding->a.nr_args, 1);
    EXPECT_STREQ(binding->a.args[0], "zzz");
    EXPECT_NULL(binding->a.args[1]);

    handle_binding(MOD_CTRL | 'A');
    const Block *block = BLOCK(buffer->blocks.next);
    ASSERT_NONNULL(block);
    ASSERT_EQ(block->size, 4);
    EXPECT_EQ(block->nl, 1);
    EXPECT_TRUE(mem_equal(block->data, "zzz\n", 4));
    EXPECT_TRUE(undo());
    EXPECT_EQ(block->size, 0);
    EXPECT_EQ(block->nl, 0);
    EXPECT_FALSE(undo());
    handle_command(&commands, "close", false);
}

static void test_posix_sanity(void)
{
    // This is not guaranteed by ISO C99, but it is required by POSIX
    // and is relied upon by this codebase:
    ASSERT_EQ(CHAR_BIT, 8);
    ASSERT_TRUE(sizeof(int) >= 4);

    IGNORE_WARNING("-Wformat-truncation")

    // Some snprintf(3) implementations historically returned -1 in case of
    // truncation. C99 and POSIX 2001 both require that it return the full
    // size of the formatted string, as if there had been enough space.
    char buf[8] = "........";
    ASSERT_EQ(snprintf(buf, 8, "0123456789"), 10);
    ASSERT_EQ(buf[7], '\0');
    EXPECT_STREQ(buf, "0123456");

    // C99 and POSIX 2001 also require the same behavior as above when the
    // size argument is 0 (and allow the buffer argument to be NULL).
    ASSERT_EQ(snprintf(NULL, 0, "987654321"), 9);

    UNIGNORE_WARNINGS
}

static void init_test_environment(void)
{
    char *home = path_absolute("build/test/HOME");
    char *dte_home = path_absolute("build/test/DTE_HOME");
    ASSERT_NONNULL(home);
    ASSERT_NONNULL(dte_home);

    ASSERT_TRUE(mkdir(home, 0755) == 0 || errno == EEXIST);
    ASSERT_TRUE(mkdir(dte_home, 0755) == 0 || errno == EEXIST);

    ASSERT_EQ(setenv("HOME", home, true), 0);
    ASSERT_EQ(setenv("DTE_HOME", dte_home, true), 0);
    ASSERT_EQ(setenv("XDG_RUNTIME_DIR", dte_home, true), 0);

    free(home);
    free(dte_home);

    ASSERT_EQ(unsetenv("TERM"), 0);
    ASSERT_EQ(unsetenv("COLORTERM"), 0);

    init_editor_state();

    const char *ver = getenv("DTE_VERSION");
    EXPECT_NONNULL(ver);
    EXPECT_STREQ(ver, editor.version);
}

static void run_tests(const TestGroup *g)
{
    const unsigned int prev_failed = failed;
    ASSERT_TRUE(g->nr_tests != 0);
    ASSERT_NONNULL(g->tests);
    for (const TestEntry *t = g->tests, *end = t + g->nr_tests; t < end; t++) {
        ASSERT_NONNULL(t->func);
        ASSERT_NONNULL(t->name);
        ASSERT_TRUE(str_has_prefix(t->name, "test_"));
        t->func();
        const char *name = t->name + STRLEN("test_");
        const unsigned int new_failed = failed - prev_failed;
        if (unlikely(new_failed)) {
            fprintf(stderr, "  FAILED  %s  [%u failures]\n", name, new_failed);
        } else {
            fprintf(stderr, "  PASSED  %s\n", name);
        }
    }
}

int main(void)
{
    test_posix_sanity();
    init_test_environment();

    run_tests(&command_tests);
    run_tests(&option_tests);
    run_tests(&editorconfig_tests);
    run_tests(&encoding_tests);
    run_tests(&filetype_tests);
    run_tests(&util_tests);
    run_tests(&terminal_tests);
    run_tests(&cmdline_tests);
    run_tests(&history_tests);
    run_tests(&syntax_tests);

    init_headless_mode();
    run_tests(&config_tests);
    test_handle_binding();
    free_syntaxes();

    fprintf(stderr, "\n  Passed: %u\n  Failed: %u\n\n", passed, failed);
    return failed ? 1 : 0;
}
