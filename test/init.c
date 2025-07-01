#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "test.h"
#include "editor.h"
#include "terminal/color.h"
#include "terminal/terminal.h"
#include "trace.h"
#include "util/fd.h"
#include "util/log.h"
#include "util/path.h"
#include "util/xreadwrite.h"
#include "version.h"

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
    // and is relied upon by this codebase.
    // See: https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/limits.h.html#tag_14_26_03_06
    ASSERT_EQ(CHAR_BIT, 8);
    ASSERT_TRUE(sizeof(int) >= 4);
    ASSERT_TRUE(BITSIZE(int) >= 32);
    ASSERT_EQ(SCHAR_MIN, -128);
    ASSERT_EQ(SCHAR_MAX, 127);
    ASSERT_EQ(UCHAR_MAX, 255);
    ASSERT_TRUE(UINT_MAX >= 0xFFFFFFFFu);
    ASSERT_TRUE(INT_MAX >= 0x7FFFFFFF);
    ASSERT_TRUE(INT_MIN <= -0x80000000);

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
    ASSERT_EQ(unsetenv("TERM"), 0);
    ASSERT_EQ(unsetenv("COLORTERM"), 0);

    EXPECT_FALSE(log_level_enabled(LOG_LEVEL_CRITICAL));
    EXPECT_FALSE(log_level_enabled(LOG_LEVEL_ERROR));
    EXPECT_FALSE(log_level_enabled(LOG_LEVEL_WARNING));
    EXPECT_FALSE(log_level_enabled(LOG_LEVEL_NOTICE));
    EXPECT_FALSE(log_level_enabled(LOG_LEVEL_INFO));
    EXPECT_FALSE(log_level_enabled(LOG_LEVEL_DEBUG));
    EXPECT_FALSE(log_level_enabled(LOG_LEVEL_TRACE));
    EXPECT_FALSE(log_level_debug_enabled());

    const LogLevel def_lvl = log_level_default();
    const LogLevel max_lvl = TRACE_LOGGING_ENABLED ? LOG_LEVEL_TRACE : def_lvl;
    const LogLevel req_lvl = LOG_LEVEL_TRACE;
    ASSERT_EQ(log_open("build/test/log.txt", req_lvl, false), max_lvl);
    EXPECT_TRUE(log_level_enabled(LOG_LEVEL_CRITICAL));
    EXPECT_TRUE(log_level_enabled(LOG_LEVEL_ERROR));
    EXPECT_TRUE(log_level_enabled(LOG_LEVEL_WARNING));
    EXPECT_TRUE(log_level_enabled(LOG_LEVEL_NOTICE));
    EXPECT_TRUE(log_level_enabled(LOG_LEVEL_INFO));
    EXPECT_EQ(log_level_enabled(LOG_LEVEL_DEBUG), DEBUG_LOGGING_ENABLED);
    EXPECT_EQ(log_level_enabled(LOG_LEVEL_TRACE), TRACE_LOGGING_ENABLED);
    EXPECT_EQ(log_level_debug_enabled(), DEBUG_LOGGING_ENABLED);
    EXPECT_FALSE(log_trace_enabled(TRACEFLAGS_ALL));
    LOG_CRITICAL("%s: testing LOG_CRITICAL()", __func__);
    LOG_ERROR("%s: testing LOG_ERROR()", __func__);
    LOG_WARNING("%s: testing LOG_WARNING()", __func__);
    LOG_NOTICE("%s: testing LOG_NOTICE()", __func__);
    LOG_INFO("%s: testing LOG_INFO()", __func__);
    LOG_DEBUG("%s: testing LOG_DEBUG()", __func__);
    LOG_TRACE(TRACEFLAGS_ALL, "should always fail; trace flags not yet set");
    log_write(LOG_LEVEL_TRACE, STRN("as above"));
    log_write(LOG_LEVEL_INFO, STRN("testing log_write()"));

    const TraceLoggingFlags all = TRACEFLAGS_ALL;
    set_trace_logging_flags(all);
    EXPECT_EQ(log_trace_enabled(all), TRACE_LOGGING_ENABLED);
    LOG_TRACE(all, "%s: testing LOG_TRACE()", __func__);
    TRACE_CMD("%s: testing TRACE_CMD()", __func__);
    TRACE_INPUT("%s: testing TRACE_INPUT()", __func__);
    TRACE_OUTPUT("%s: testing TRACE_OUTPUT()", __func__);

    EditorState *e = init_editor_state(home, dte_home);
    ASSERT_NONNULL(e);
    ASSERT_NONNULL(e->user_config_dir);
    ASSERT_NONNULL(e->home_dir.data);
    EXPECT_STREQ(e->user_config_dir, dte_home);
    EXPECT_STRVIEW_EQ_CSTRING(&e->home_dir, home);
    free(home);
    free(dte_home);
    e->options.lock_files = false;
    ctx->userdata = e;

    Terminal *term = &e->terminal;
    EXPECT_EQ(term->features, 0);
    EXPECT_EQ(term->width, 0);
    EXPECT_EQ(term->height, 0);
    term_init(term, "decansi", NULL);
    EXPECT_EQ(term->features, TFLAG_8_COLOR);
    EXPECT_EQ(term->width, 80);
    EXPECT_EQ(term->height, 24);

    const char *ver = getenv("DTE_VERSION");
    EXPECT_NONNULL(ver);
    EXPECT_STREQ(ver, VERSION);
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
    EXPECT_FALSE(e->err.print_to_stderr);
    EXPECT_FALSE(e->err.stderr_errors_printed);
    EXPECT_EQ(e->flags, EFLAG_HEADLESS);
    EXPECT_EQ(e->include_recursion_count, 0);
    EXPECT_FALSE(e->options.lock_files);

    frame_remove(e, e->root_frame);
    EXPECT_NULL(e->view);
    EXPECT_NULL(e->buffer);
    EXPECT_EQ(e->buffers.count, 0);

    free_editor_state(e);
    EXPECT_TRUE(log_close());
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

const TestGroup init_tests = TEST_GROUP(itests);
const TestGroup deinit_tests = TEST_GROUP(dtests);
