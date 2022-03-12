#include <stdlib.h>
#include <unistd.h>
#include "test.h"
#include "config.h"
#include "command/macro.h"
#include "command/run.h"
#include "commands.h"
#include "convert.h"
#include "editor.h"
#include "error.h"
#include "frame.h"
#include "misc.h"
#include "mode.h"
#include "syntax/state.h"
#include "syntax/syntax.h"
#include "terminal/terminal.h"
#include "util/debug.h"
#include "util/path.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/xsnprintf.h"
#include "window.h"
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
            EXPECT_NULL(find_syntax(&editor.syntaxes, path_basename(cfg.name)));
            int err;
            ConfigFlags flags = CFG_BUILTIN | CFG_MUST_EXIST;
            unsigned int saved_nr_errs = get_nr_errors();
            EXPECT_NONNULL(load_syntax_file(cfg.name, flags, &err));
            EXPECT_EQ(get_nr_errors(), saved_nr_errs);
            EXPECT_NONNULL(find_syntax(&editor.syntaxes, path_basename(cfg.name)));
        } else {
            // Check that built-in configs are identical to their source files
            char path[4096];
            xsnprintf(path, sizeof path, "config/%s", cfg.name);
            char *src;
            ssize_t size = read_file(path, &src);
            ASSERT_EQ(size, cfg.text.length);
            EXPECT_MEMEQ(src, cfg.text.data, size);
            free(src);
        }
    }
    update_all_syntax_colors(&editor.syntaxes);
}

static void expect_files_equal(const char *path1, const char *path2)
{
    char *buf1;
    ssize_t size1 = read_file(path1, &buf1);
    if (size1 < 0) {
        TEST_FAIL("Error reading '%s': %s", path1, strerror(errno));
        return;
    }
    passed++;

    char *buf2;
    ssize_t size2 = read_file(path2, &buf2);
    if (size2 < 0) {
        free(buf1);
        TEST_FAIL("Error reading '%s': %s", path2, strerror(errno));
        return;
    }
    passed++;

    if (size1 != size2 || !mem_equal(buf1, buf2, size1)) {
        TEST_FAIL("Files differ: '%s', '%s'", path1, path2);
    } else {
        passed++;
    }

    free(buf1);
    free(buf2);
}

static void test_exec_config(void)
{
    static const char *const outfiles[] = {
        "env.txt",
        "crlf.txt",
        "thai-utf8.txt",
        "pipe-from.txt",
        "pipe-to.txt",
        "redo1.txt",
        "redo2.txt",
        "shift.txt",
    };

    // Delete output files left over from previous runs
    unlink("build/test/thai-tis620.txt");
    FOR_EACH_I(i, outfiles) {
        char out[64];
        xsnprintf(out, sizeof out, "build/test/%s", outfiles[i]);
        unlink(out);
    }

    // Execute *.dterc files
    const CommandSet *cmds = &normal_commands;
    FOR_EACH_I(i, builtin_configs) {
        const BuiltinConfig config = builtin_configs[i];
        exec_config(cmds, config.text);
    }

    // Check that output files have expected contents
    FOR_EACH_I(i, outfiles) {
        char out[64], ref[64];
        xsnprintf(out, sizeof out, "build/test/%s", outfiles[i]);
        xsnprintf(ref, sizeof ref, "test/data/%s", outfiles[i]);
        expect_files_equal(out, ref);
    }

    if (conversion_supported_by_iconv("UTF-8", "TIS-620")) {
        expect_files_equal("build/test/thai-tis620.txt", "test/data/thai-tis620.txt");
    }

    const StringView s = STRING_VIEW("toggle utf8-bom \\");
    EXPECT_FALSE(editor.options.utf8_bom);
    exec_config(cmds, s);
    EXPECT_TRUE(editor.options.utf8_bom);
    exec_config(cmds, s);
    EXPECT_FALSE(editor.options.utf8_bom);
}

static void test_detect_indent(void)
{
    const CommandSet *cmds = &normal_commands;
    EXPECT_FALSE(editor.options.detect_indent);
    EXPECT_FALSE(editor.options.expand_tab);
    EXPECT_EQ(editor.options.indent_width, 8);

    handle_command (
        cmds,
        "option -r '/test/data/detect-indent\\.ini$' detect-indent 2,4,8;"
        "open test/data/detect-indent.ini",
        false
    );

    EXPECT_EQ(editor.buffer->options.detect_indent, 1 << 1 | 1 << 3 | 1 << 7);
    EXPECT_TRUE(editor.buffer->options.expand_tab);
    EXPECT_EQ(editor.buffer->options.indent_width, 2);

    handle_command(cmds, "close", false);
}

static void test_global_state(void)
{
    const Buffer *buffer = editor.buffer;
    const View *view = editor.view;
    const Window *window = editor.window;
    const Frame *root_frame = editor.root_frame;
    ASSERT_NONNULL(window);
    ASSERT_NONNULL(root_frame);
    ASSERT_NONNULL(buffer);
    ASSERT_NONNULL(view);
    ASSERT_PTREQ(window->view, view);
    ASSERT_PTREQ(window->frame, root_frame);
    ASSERT_PTREQ(view->buffer, buffer);
    ASSERT_PTREQ(view->window, window);
    ASSERT_PTREQ(root_frame->window, window);
    ASSERT_PTREQ(root_frame->parent, NULL);

    ASSERT_EQ(window->views.count, 1);
    ASSERT_EQ(buffer->views.count, 1);
    ASSERT_EQ(editor.buffers.count, 1);
    ASSERT_PTREQ(window->views.ptrs[0], view);
    ASSERT_PTREQ(buffer->views.ptrs[0], view);
    ASSERT_PTREQ(editor.buffers.ptrs[0], buffer);

    ASSERT_NONNULL(buffer->encoding.name);
    ASSERT_NONNULL(buffer->blocks.next);
    ASSERT_PTREQ(&buffer->blocks, view->cursor.head);
    ASSERT_PTREQ(buffer->blocks.next, view->cursor.blk);
    ASSERT_PTREQ(buffer->cur_change, &buffer->change_head);
    ASSERT_PTREQ(buffer->saved_change, buffer->cur_change);
    EXPECT_NULL(buffer->display_filename);

    // Note: this isn't necessarily equal to 1 because some UNITTEST
    // blocks may have already called window_open_empty_buffer()
    // before init_headless_mode() was entered
    EXPECT_TRUE(buffer->id > 0);
}

static void test_macro_record(void)
{
    EditorState *e = &editor;
    EXPECT_EQ(e->input_mode, INPUT_NORMAL);
    EXPECT_FALSE(macro_is_recording());
    macro_record();
    EXPECT_TRUE(macro_is_recording());

    const CommandSet *cmds = &normal_commands;
    handle_command(cmds, "open", false);
    handle_input(e, 'x');
    handle_input(e, 'y');
    handle_command(cmds, "bol", true);
    handle_input(e, '-');
    handle_input(e, 'z');
    handle_command(cmds, "eol; right; insert -m .; new-line", true);

    const StringView t1 = STRING_VIEW("test 1\n");
    insert_text(e->view, t1.data, t1.length, true);
    macro_insert_text_hook(t1.data, t1.length);

    const StringView t2 = STRING_VIEW("-- test 2");
    insert_text(e->view, t2.data, t2.length, true);
    macro_insert_text_hook(t2.data, t2.length);

    EXPECT_TRUE(macro_is_recording());
    macro_stop();
    EXPECT_FALSE(macro_is_recording());

    handle_command (
        cmds,
        "save -f build/test/macro-rec.txt;"
        "close -f;"
        "open;"
        "macro play;"
        "save -f build/test/macro-out.txt;"
        "close -f;"
        "show macro;"
        "close -f;",
        true
    );

    expect_files_equal("build/test/macro-rec.txt", "build/test/macro-out.txt");
}

static const TestEntry tests[] = {
    TEST(test_global_state),
    TEST(test_builtin_configs),
    TEST(test_exec_config),
    TEST(test_detect_indent),
    TEST(test_macro_record),
};

const TestGroup config_tests = TEST_GROUP(tests);

DISABLE_WARNING("-Wmissing-prototypes")

void init_headless_mode(void)
{
    MEMZERO(&editor.terminal.control_codes);
    exec_builtin_rc(&editor.colors, TERM_8_COLOR);
    update_all_syntax_colors(&editor.syntaxes);
    editor.options.lock_files = false;
    editor.window = new_window();
    editor.root_frame = new_root_frame(editor.window);
    set_view(&editor, window_open_empty_buffer(editor.window));
}
