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
#include "util/fd.h"
#include "util/log.h"
#include "util/path.h"
#include "util/xreadwrite.h"

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
    LOG_DEBUG("%s: testing LOG_DEBUG()", __func__);
    LOG_TRACE("%s: testing LOG_TRACE()", __func__);
    log_write(LOG_LEVEL_WARNING, STRN("testing log_write()"));
    log_write(LOG_LEVEL_INFO, STRN("testing log_write()"));

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
