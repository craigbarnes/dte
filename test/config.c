#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test.h"
#include "../src/config.h"
#include "../src/debug.h"
#include "../src/editor.h"
#include "../src/frame.h"
#include "../src/encoding/convert.h"
#include "../src/terminal/no-op.h"
#include "../src/terminal/terminal.h"
#include "../src/util/readfile.h"
#include "../src/util/str-util.h"
#include "../src/util/string-view.h"
#include "../src/window.h"
#include "../build/test/data.h"

DISABLE_WARNING("-Wmissing-prototypes")

static const char extra_rc[] =
    "set lock-files false\n"
    // Regression test for unquoted variables in rc files
    "bind M-p \"insert \"$WORD\n"
    "bind M-p \"insert \"$FILE\n"
;

void init_headless_mode(void)
{
    MEMZERO(&terminal.control_codes);
    terminal.cooked = &no_op;
    terminal.raw = &no_op;
    editor.resize = &no_op;
    editor.ui_end = &no_op;

    exec_reset_colors_rc();
    read_config(commands, "rc", CFG_MUST_EXIST | CFG_BUILTIN);
    fill_builtin_colors();

    window = new_window();
    root_frame = new_root_frame(window);

    exec_config(commands, extra_rc, sizeof(extra_rc) - 1);

    set_view(window_open_empty_buffer(window));
}

static void expect_files_equal(const char *path1, const char *path2)
{
    char *buf1;
    ssize_t size1 = read_file(path1, &buf1);
    if (size1 < 0) {
        TEST_FAIL("Error reading '%s': %s", path1, strerror(errno));
        return;
    }

    char *buf2;
    ssize_t size2 = read_file(path2, &buf2);
    if (size2 < 0) {
        free(buf1);
        TEST_FAIL("Error reading '%s': %s", path2, strerror(errno));
        return;
    }

    if (size1 != size2 || memcmp(buf1, buf2, size1) != 0) {
        TEST_FAIL("Files differ: '%s', '%s'", path1, path2);
    }

    free(buf1);
    free(buf2);
}

void test_exec_config(void)
{
    ASSERT_NONNULL(window);
    FOR_EACH_I(i, builtin_configs) {
        const BuiltinConfig config = builtin_configs[i];
        exec_config(commands, config.text.data, config.text.length);
    }

    expect_files_equal("build/test/env.txt", "test/data/env.txt");
    expect_files_equal("build/test/crlf.txt", "test/data/crlf.txt");
    expect_files_equal("build/test/thai-utf8.txt", "test/data/thai-utf8.txt");
    EXPECT_EQ(unlink("build/test/env.txt"), 0);
    EXPECT_EQ(unlink("build/test/crlf.txt"), 0);
    EXPECT_EQ(unlink("build/test/thai-utf8.txt"), 0);

    if (encoding_supported_by_iconv("TIS-620")) {
        expect_files_equal("build/test/thai-tis620.txt", "test/data/thai-tis620.txt");
        EXPECT_EQ(unlink("build/test/thai-tis620.txt"), 0);
    }
}
