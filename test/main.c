#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "test.h"
#include "editor.h"
#include "syntax/syntax.h"
#include "util/path.h"

void init_headless_mode(void);
extern const TestGroup bind_tests;
extern const TestGroup cmdline_tests;
extern const TestGroup command_tests;
extern const TestGroup config_tests;
extern const TestGroup dump_tests;
extern const TestGroup editorconfig_tests;
extern const TestGroup encoding_tests;
extern const TestGroup filetype_tests;
extern const TestGroup history_tests;
extern const TestGroup option_tests;
extern const TestGroup syntax_tests;
extern const TestGroup terminal_tests;
extern const TestGroup util_tests;

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

static void test_init(void)
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
    ASSERT_TRUE(g->nr_tests != 0);
    ASSERT_NONNULL(g->tests);

    for (const TestEntry *t = g->tests, *end = t + g->nr_tests; t < end; t++) {
        ASSERT_NONNULL(t->func);
        ASSERT_NONNULL(t->name);
        unsigned int prev_failed = failed;
        unsigned int prev_passed = passed;
        t->func();
        unsigned int f = failed - prev_failed;
        unsigned int p = passed - prev_passed;
        fprintf(stderr, "   CHECK  %-35s  %4u passed", t->name, p);
        if (unlikely(f > 0)) {
            fprintf(stderr, " %4u FAILED", f);
        }
        fputc('\n', stderr);
    }
}

static const TestEntry tests[] = {
    TEST(test_posix_sanity),
    TEST(test_init),
};

const TestGroup init_tests = TEST_GROUP(tests);

int main(void)
{
    run_tests(&init_tests);
    run_tests(&command_tests);
    run_tests(&option_tests);
    run_tests(&editorconfig_tests);
    run_tests(&encoding_tests);
    run_tests(&filetype_tests);
    run_tests(&util_tests);
    run_tests(&terminal_tests);
    run_tests(&cmdline_tests);
    run_tests(&history_tests);

    init_headless_mode();
    run_tests(&config_tests);
    run_tests(&bind_tests);
    run_tests(&syntax_tests);
    run_tests(&dump_tests);
    free_syntaxes();

    fprintf(stderr, "\n   TOTAL  %u passed, %u failed\n\n", passed, failed);
    return failed ? 1 : 0;
}
