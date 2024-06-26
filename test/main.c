#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "test.h"
#include "editor.h"
#include "syntax/syntax.h"
#include "util/exitcode.h"
#include "util/fd.h"
#include "util/log.h"
#include "util/path.h"
#include "util/progname.h"
#include "util/str-util.h"
#include "util/time-util.h"
#include "util/xreadwrite.h"

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
extern const TestGroup error_tests;
extern const TestGroup filetype_tests;
extern const TestGroup frame_tests;
extern const TestGroup history_tests;
extern const TestGroup indent_tests;
extern const TestGroup option_tests;
extern const TestGroup shift_tests;
extern const TestGroup spawn_tests;
extern const TestGroup status_tests;
extern const TestGroup syntax_tests;
extern const TestGroup terminal_tests;
extern const TestGroup util_tests;

static bool fd_is_valid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

static void test_process_sanity(TestContext *ctx)
{
    // Note that if this fails, there'll only be (at best) an "aborted"
    // message, since there's no stderr for test_fail() to use
    ASSERT_TRUE(fd_is_valid(STDERR_FILENO));

    ASSERT_NONNULL(freopen("/dev/null", "r", stdin));
    ASSERT_NONNULL(freopen("/dev/null", "w", stdout));
}

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

// Note: this must be ordered before a successful call to log_open()
// (i.e. the one in test_init()), so as not to trigger the BUG_ON()
// assertions at the top of log_open()
static void test_log_open_errors(TestContext *ctx)
{
    const LogLevel none = LOG_LEVEL_NONE;
    const LogLevel info = LOG_LEVEL_INFO;
    errno = 0;
    ASSERT_EQ(log_open("/dev/null", none, false), none);
    EXPECT_EQ(errno, 0);
    ASSERT_EQ(log_open("build/test/non-existent-dir/x", info, false), none);
    EXPECT_EQ(errno, ENOENT);

    if (access("/dev/full", W_OK) == 0) {
        errno = 0;
        ASSERT_EQ(log_open("/dev/full", info, false), none);
        EXPECT_EQ(errno, ENOSPC);
    }

    int tty = open("/dev/tty", O_WRONLY | O_CLOEXEC | O_NOCTTY);
    if (tty >= 0) {
        ASSERT_TRUE(tty > STDERR_FILENO);
        ASSERT_TRUE(is_controlling_tty(tty));
        EXPECT_EQ(xclose(tty), 0);
        errno = 0;
        ASSERT_EQ(log_open("/dev/tty", info, false), none);
        EXPECT_EQ(errno, EINVAL);
    }
}

static void test_init(TestContext *ctx)
{
    char *home = path_absolute("build/test/HOME");
    char *dte_home = path_absolute("build/test/DTE_HOME");
    ASSERT_NONNULL(home);
    ASSERT_NONNULL(dte_home);

    ASSERT_TRUE(mkdir(home, 0755) == 0 || errno == EEXIST);
    ASSERT_TRUE(mkdir(dte_home, 0755) == 0 || errno == EEXIST);

    ASSERT_EQ(setenv("HOME", home, 1), 0);
    ASSERT_EQ(setenv("DTE_HOME", dte_home, 1), 0);
    ASSERT_EQ(setenv("XDG_RUNTIME_DIR", dte_home, 1), 0);
    ASSERT_EQ(setenv("TZ", "UTC", 1), 0);

    free(home);
    free(dte_home);

    ASSERT_EQ(unsetenv("TERM"), 0);
    ASSERT_EQ(unsetenv("COLORTERM"), 0);

    LogLevel lvl = LOG_LEVEL_WARNING;
    ASSERT_EQ(log_open("build/test/log.txt", lvl, false), lvl);
    LOG_CRITICAL("%s: testing LOG_CRITICAL()", __func__);
    LOG_ERROR("%s: testing LOG_ERROR()", __func__);
    LOG_WARNING("%s: testing LOG_WARNING()", __func__);
    LOG_NOTICE("%s: testing LOG_NOTICE()", __func__);
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
    EXPECT_EQ(term->ncv_attributes, 0);
    EXPECT_EQ(term->features, TFLAG_8_COLOR);
    EXPECT_EQ(term->ibuf.len, 0);
    EXPECT_NONNULL(term->ibuf.buf);

    // Make sure no terminal output was buffered
    const TermOutputBuffer *obuf = &term->obuf;
    EXPECT_NONNULL(obuf->buf);
    EXPECT_EQ(obuf->count, 0);
    EXPECT_EQ(obuf->scroll_x, 0);
    EXPECT_EQ(obuf->x, 0);
    EXPECT_EQ(obuf->width, 0);
    EXPECT_EQ(obuf->style.fg, COLOR_DEFAULT);
    EXPECT_EQ(obuf->style.bg, COLOR_DEFAULT);
    EXPECT_EQ(obuf->style.attr, 0);
    EXPECT_EQ(obuf->cursor_style.type, CURSOR_DEFAULT);
    EXPECT_EQ(obuf->cursor_style.color, COLOR_DEFAULT);

    ASSERT_NONNULL(e->view);
    ASSERT_NONNULL(e->buffer);
    EXPECT_EQ(e->buffers.count, 1);
    EXPECT_NULL(e->buffer->abs_filename);
    EXPECT_PTREQ(e->buffer, e->buffers.ptrs[0]);
    EXPECT_FALSE(e->child_controls_terminal);
    EXPECT_EQ(e->flags, 0);

    frame_remove(e, e->root_frame);
    EXPECT_NULL(e->view);
    EXPECT_NULL(e->buffer);
    EXPECT_EQ(e->buffers.count, 0);

    free_editor_state(e);
    EXPECT_TRUE(log_close());
}

static void print_timing(const TestContext *ctx, const struct timespec ts[2])
{
    struct timespec diff;
    timespec_subtract(&ts[1], &ts[0], &diff);

    double ms = (double)diff.tv_sec * (double)MS_PER_SECOND;
    ms += (double)diff.tv_nsec / (double)NS_PER_MS;

    int precision = 3 - (ms >= 1000.0) - (ms >= 100.0) - (ms >= 10.0);
    fprintf(stderr, " %s(%0.*f ms)%s", ctx->dim, precision, ms, ctx->sgr0);
}

static void run_tests(TestContext *ctx, const TestGroup *g)
{
    // Note: we avoid using test.h assertions in this function, in order to
    // avoid artificially inflating ctx->passed
    if (unlikely(g->nr_tests == 0 || !g->tests)) {
        fputs("Error: TestGroup with no entries\n", stderr);
        abort();
    }

    bool timing = ctx->timing;

    for (const TestEntry *t = g->tests, *end = t + g->nr_tests; t < end; t++) {
        if (unlikely(!t->name || !t->func)) {
            fputs("Error: TestEntry with NULL name or function\n", stderr);
            abort();
        }

        unsigned int prev_passed = ctx->passed;
        unsigned int prev_failed = ctx->failed;
        struct timespec times[2];
        timing = timing && !clock_gettime(CLOCK_MONOTONIC, &times[0]);
        t->func(ctx);
        timing = timing && !clock_gettime(CLOCK_MONOTONIC, &times[1]);
        unsigned int passed = ctx->passed - prev_passed;
        unsigned int failed = ctx->failed - prev_failed;

        fprintf(stderr, "   CHECK  %-35s  %5u passed", t->name, passed);

        if (unlikely(failed > 0)) {
            fprintf(stderr, " %s%4u FAILED%s", ctx->boldred, failed, ctx->sgr0);
        }

        if (timing) {
            print_timing(ctx, times);
        }

        fputc('\n', stderr);
    }

    ctx->nfuncs += g->nr_tests;
    ctx->ngroups += 1;
}

static const TestEntry itests[] = {
    TEST(test_process_sanity),
    TEST(test_posix_sanity),
    TEST(test_log_open_errors),
    TEST(test_init),
};

static const TestEntry dtests[] = {
    TEST(test_deinit),
};

static const TestGroup init_tests = TEST_GROUP(itests);
static const TestGroup deinit_tests = TEST_GROUP(dtests);

// Change to the expected working directory, in case the test binary
// wasn't executed via `make check` (or `build/test/test`).
static void init_working_directory(int argc, char *argv[])
{
    static const char testfile[] = "test/data/crlf.dterc";
    if (likely(access(testfile, F_OK) == 0)) {
        // Working directory already correct
        return;
    }

    if (argc < 1 || !argv || !argv[0]) {
        goto error;
    }

    char dir[8192];
    StringView progdir = path_slice_dirname(argv[0]);
    if (progdir.length >= sizeof(dir) - 8) {
        goto error;
    }

    memcpy(dir, progdir.data, progdir.length);
    copyliteral(dir + progdir.length, "/../..\0");

    if (chdir(dir) != 0) {
        perror("chdir");
        goto error;
    }

    if (access(testfile, F_OK) != 0) {
        goto error;
    }

    return;

error:
    fputs("test binary executed from incorrect working directory; exiting", stderr);
    exit(1);
}

static void usage(FILE *stream, int argc, char *argv[])
{
    fprintf(stream, "Usage: %s [-cCth]\n", progname(argc, argv, NULL));
}

int main(int argc, char *argv[])
{
    TestContext ctx = {
        .timing = false,
        .boldred = "\033[1;31m",
        .dim = "\033[2m",
        .sgr0 = "\033[0m",
    };

    bool color = isatty(STDERR_FILENO) && !xgetenv("NO_COLOR");

    for (int ch; (ch = getopt(argc, argv, "cCth")) != -1; ) {
        switch (ch) {
        case 'c':
        case 'C':
            color = (ch == 'c');
            break;
        case 't':
            ctx.timing = true;
            break;
        case 'h':
            usage(stdout, argc, argv);
            return EC_OK;
        default:
            usage(stderr, argc, argv);
            return EC_USAGE_ERROR;
        }
    }

    setvbuf(stderr, NULL, _IOLBF, 0);
    init_working_directory(argc, argv);

    if (!color) {
        ctx.boldred[0] = '\0';
        ctx.dim[0] = '\0';
        ctx.sgr0[0] = '\0';
    }

    struct timespec times[2];
    bool timing = ctx.timing && !clock_gettime(CLOCK_MONOTONIC, &times[0]);

    run_tests(&ctx, &init_tests);
    run_tests(&ctx, &util_tests);
    run_tests(&ctx, &indent_tests);
    run_tests(&ctx, &command_tests);
    run_tests(&ctx, &option_tests);
    run_tests(&ctx, &editorconfig_tests);
    run_tests(&ctx, &encoding_tests);
    run_tests(&ctx, &filetype_tests);
    run_tests(&ctx, &terminal_tests);
    run_tests(&ctx, &cmdline_tests);
    run_tests(&ctx, &history_tests);
    run_tests(&ctx, &frame_tests);
    run_tests(&ctx, &ctags_tests);
    run_tests(&ctx, &spawn_tests);
    run_tests(&ctx, &status_tests);

    init_headless_mode(&ctx);
    run_tests(&ctx, &config_tests);
    run_tests(&ctx, &bind_tests);
    run_tests(&ctx, &cmdline_tests_late);
    run_tests(&ctx, &buffer_tests);
    run_tests(&ctx, &shift_tests);
    run_tests(&ctx, &bookmark_tests);
    run_tests(&ctx, &syntax_tests);
    run_tests(&ctx, &dump_tests);
    run_tests(&ctx, &error_tests);
    run_tests(&ctx, &deinit_tests);

    timing = timing && !clock_gettime(CLOCK_MONOTONIC, &times[1]);

    fprintf (
        stderr,
        "\n   TOTAL  %u passed, %s%u failed%s",
        ctx.passed,
        ctx.failed ? ctx.boldred : "", ctx.failed, ctx.sgr0
    );

    if (timing) {
        fprintf(stderr, " (%u functions, %u groups)", ctx.nfuncs, ctx.ngroups);
        print_timing(&ctx, times);
    }

    fputs("\n\n", stderr);
    return ctx.failed ? 1 : 0;
}
