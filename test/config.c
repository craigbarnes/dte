#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test.h"
#include "../src/config.h"
#include "../src/debug.h"
#include "../src/editor.h"
#include "../src/encoding/convert.h"
#include "../src/error.h"
#include "../src/frame.h"
#include "../src/syntax/state.h"
#include "../src/terminal/no-op.h"
#include "../src/terminal/terminal.h"
#include "../src/util/path.h"
#include "../src/util/readfile.h"
#include "../src/util/str-util.h"
#include "../src/util/string-view.h"
#include "../src/util/xsnprintf.h"
#include "../src/window.h"
#include "../build/test/data.h"

static void test_builtin_configs(void)
{
    size_t n;
    const BuiltinConfig *editor_builtin_configs = get_builtin_configs_array(&n);
    for (size_t i = 0; i < n; i++) {
        const BuiltinConfig cfg = editor_builtin_configs[i];
        if (str_has_prefix(cfg.name, "syntax/")) {
            if (str_has_prefix(cfg.name, "syntax/inc/")) {
                continue;
            }
            // Check that built-in syntax files load without errors
            EXPECT_NULL(find_syntax(path_basename(cfg.name)));
            int err;
            ConfigFlags flags = CFG_BUILTIN | CFG_MUST_EXIST;
            unsigned int saved_nr_errs = get_nr_errors();
            EXPECT_NONNULL(load_syntax_file(cfg.name, flags, &err));
            EXPECT_EQ(get_nr_errors(), saved_nr_errs);
            EXPECT_NONNULL(find_syntax(path_basename(cfg.name)));
        } else {
            // Check that built-in configs are identical to their source files
            const char *name = cfg.name;
            char *src, *path = xasprintf("config/%s", name);
            ssize_t size = read_file(path, &src);
            free(path);
            ASSERT_EQ(size, cfg.text.length);
            EXPECT_TRUE(mem_equal(src, cfg.text.data, size));
            free(src);
        }
    }
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

    if (size1 != size2 || !mem_equal(buf1, buf2, size1)) {
        TEST_FAIL("Files differ: '%s', '%s'", path1, path2);
    }

    free(buf1);
    free(buf2);
}

static void test_exec_config(void)
{
    FOR_EACH_I(i, builtin_configs) {
        const BuiltinConfig config = builtin_configs[i];
        exec_config(&commands, config.text.data, config.text.length);
    }

    const char *outfiles[] = {
        "env.txt",
        "crlf.txt",
        "thai-utf8.txt",
        "pipe-from.txt",
        "pipe-to.txt",
    };

    // Check that files created by test configs have expected contents
    FOR_EACH_I(i, outfiles) {
        char out[64], ref[64];
        xsnprintf(out, sizeof out, "build/test/%s", outfiles[i]);
        xsnprintf(ref, sizeof ref, "test/data/%s", outfiles[i]);
        expect_files_equal(out, ref);
        EXPECT_EQ(unlink(out), 0);
    }

    if (encoding_supported_by_iconv("TIS-620")) {
        expect_files_equal("build/test/thai-tis620.txt", "test/data/thai-tis620.txt");
        EXPECT_EQ(unlink("build/test/thai-tis620.txt"), 0);
    }
}

DISABLE_WARNING("-Wmissing-prototypes")

void init_headless_mode(void)
{
    const char extra_rc[] =
        "set lock-files false\n"
        // Regression test for unquoted variables in rc files
        "bind M-p \"insert \"$WORD\n"
        "bind M-p \"insert \"$FILE\n"
    ;

    MEMZERO(&terminal.control_codes);
    editor.resize = &no_op;
    editor.ui_end = &no_op;
    exec_builtin_rc();
    window = new_window();
    root_frame = new_root_frame(window);
    exec_config(&commands, extra_rc, sizeof(extra_rc) - 1);
    set_view(window_open_empty_buffer(window));
}

void test_config(void)
{
    ASSERT_NONNULL(window);
    test_builtin_configs();
    test_exec_config();
}
