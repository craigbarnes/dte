#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "test.h"
#include "editor.h"
#include "syntax/syntax.h"
#include "util/log.h"
#include "util/path.h"

void init_headless_mode(TestContext *ctx);
extern const TestGroup bind_tests;
extern const TestGroup bookmark_tests;
extern const TestGroup buffer_tests;
extern const TestGroup cmdline_tests;
extern const TestGroup cmdline_tests_late;
extern const TestGroup command_tests;
extern const TestGroup config_tests;
extern const TestGroup ctags_tests;
extern const TestGroup dump_tests;
extern const TestGroup editorconfig_tests;
extern const TestGroup encoding_tests;
extern const TestGroup filetype_tests;
extern const TestGroup history_tests;
extern const TestGroup option_tests;
extern const TestGroup spawn_tests;
extern const TestGroup status_tests;
extern const TestGroup syntax_tests;
extern const TestGroup terminal_tests;
extern const TestGroup util_tests;

static void test_posix_sanity(TestContext *ctx)
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

static void test_init(TestContext *ctx)
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

    LogLevel lvl = LOG_LEVEL_WARNING;
    ASSERT_EQ(log_open("build/test/log.txt", lvl), lvl);
    LOG_ERROR("%s: testing LOG_ERROR()", __func__);
    LOG_WARNING("%s: testing LOG_WARNING()", __func__);
    LOG_INFO("%s: testing LOG_INFO()", __func__);

    EditorState *e = init_editor_state();
    ctx->userdata = e;
    ASSERT_NONNULL(e);
    ASSERT_NONNULL(e->user_config_dir);
    ASSERT_NONNULL(e->home_dir.data);
    EXPECT_NONNULL(e->terminal.obuf.buf);
    EXPECT_NONNULL(e->terminal.ibuf.buf);

    const char *ver = getenv("DTE_VERSION");
    EXPECT_NONNULL(ver);
    EXPECT_STREQ(ver, e->version);
}

static void test_deinit(TestContext *ctx)
{
    // Make sure default Terminal state wasn't changed
    EditorState *e = ctx->userdata;
    const Terminal *term = &e->terminal;
    EXPECT_EQ(term->width, 80);
    EXPECT_EQ(term->height, 24);
    EXPECT_EQ(term->color_type, TERM_8_COLOR);
    EXPECT_EQ(term->ncv_attributes, 0);
    EXPECT_EQ(term->features, 0);
    EXPECT_EQ(term->ibuf.len, 0);
    EXPECT_NONNULL(term->ibuf.buf);

    // Make sure no terminal output was buffered
    const TermOutputBuffer *obuf = &term->obuf;
    EXPECT_NONNULL(obuf->buf);
    EXPECT_EQ(obuf->count, 0);
    EXPECT_EQ(obuf->scroll_x, 0);
    EXPECT_EQ(obuf->x, 0);
    EXPECT_EQ(obuf->width, 0);
    EXPECT_EQ(obuf->color.fg, 0);
    EXPECT_EQ(obuf->color.bg, 0);
    EXPECT_EQ(obuf->color.attr, 0);
    EXPECT_EQ(obuf->cursor_style.type, 0);
    EXPECT_EQ(obuf->cursor_style.color, 0);

    ASSERT_NONNULL(e->view);
    ASSERT_NONNULL(e->buffer);
    EXPECT_EQ(e->buffers.count, 1);
    EXPECT_NULL(e->buffer->abs_filename);
    EXPECT_PTREQ(e->buffer, e->buffers.ptrs[0]);
    EXPECT_FALSE(e->child_controls_terminal);

    remove_frame(e, e->root_frame);
    EXPECT_NULL(e->view);
    EXPECT_NULL(e->buffer);
    EXPECT_EQ(e->buffers.count, 0);

    free_editor_state(e);
    EXPECT_EQ(e->colors.other.count, 0);
    EXPECT_EQ(e->colors.other.mask, 0);
    EXPECT_NULL(e->colors.other.entries);
}

static void run_tests(TestContext *ctx, const TestGroup *g)
{
    ASSERT_TRUE(g->nr_tests != 0);
    ASSERT_NONNULL(g->tests);

    for (const TestEntry *t = g->tests, *end = t + g->nr_tests; t < end; t++) {
        ASSERT_NONNULL(t->func);
        ASSERT_NONNULL(t->name);
        unsigned int prev_failed = ctx->failed;
        unsigned int prev_passed = ctx->passed;
        t->func(ctx);
        unsigned int f = ctx->failed - prev_failed;
        unsigned int p = ctx->passed - prev_passed;
        fprintf(stderr, "   CHECK  %-35s  %4u passed", t->name, p);
        if (unlikely(f > 0)) {
            fprintf(stderr, " %4u FAILED", f);
        }
        fputc('\n', stderr);
    }
}

static const TestEntry itests[] = {
    TEST(test_posix_sanity),
    TEST(test_init),
};

static const TestEntry dtests[] = {
    TEST(test_deinit),
};

static const TestGroup init_tests = TEST_GROUP(itests);
static const TestGroup deinit_tests = TEST_GROUP(dtests);

int main(void)
{
    TestContext ctx = {
        .passed = 0,
        .failed = 0,
    };

    run_tests(&ctx, &init_tests);
    run_tests(&ctx, &command_tests);
    run_tests(&ctx, &option_tests);
    run_tests(&ctx, &editorconfig_tests);
    run_tests(&ctx, &encoding_tests);
    run_tests(&ctx, &filetype_tests);
    run_tests(&ctx, &util_tests);
    run_tests(&ctx, &terminal_tests);
    run_tests(&ctx, &cmdline_tests);
    run_tests(&ctx, &history_tests);
    run_tests(&ctx, &ctags_tests);
    run_tests(&ctx, &spawn_tests);
    run_tests(&ctx, &status_tests);

    init_headless_mode(&ctx);
    run_tests(&ctx, &config_tests);
    run_tests(&ctx, &bind_tests);
    run_tests(&ctx, &cmdline_tests_late);
    run_tests(&ctx, &buffer_tests);
    run_tests(&ctx, &bookmark_tests);
    run_tests(&ctx, &syntax_tests);
    run_tests(&ctx, &dump_tests);
    run_tests(&ctx, &deinit_tests);

    fprintf(stderr, "\n   TOTAL  %u passed, %u failed\n\n", ctx.passed, ctx.failed);
    return ctx.failed ? 1 : 0;
}
