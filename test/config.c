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
#include "insert.h"
#include "misc.h"
#include "mode.h"
#include "syntax/state.h"
#include "syntax/syntax.h"
#include "terminal/terminal.h"
#include "test-data.h"
#include "util/debug.h"
#include "util/path.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/xsnprintf.h"
#include "window.h"

static void test_builtin_configs(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    HashMap *syntaxes = &e->syntaxes;
    size_t n;
    const BuiltinConfig *editor_builtin_configs = get_builtin_configs_array(&n);
    e->err->print_to_stderr = true;

    for (size_t i = 0; i < n; i++) {
        const BuiltinConfig cfg = editor_builtin_configs[i];
        if (str_has_prefix(cfg.name, "syntax/")) {
            if (str_has_prefix(cfg.name, "syntax/inc/")) {
                continue;
            }
            // Check that built-in syntax files load without errors
            EXPECT_NULL(find_syntax(syntaxes, path_basename(cfg.name)));
            int err;
            SyntaxLoadFlags flags = SYN_BUILTIN | SYN_MUST_EXIST;
            unsigned int saved_nr_errs = e->err->nr_errors;
            EXPECT_NONNULL(load_syntax_file(e, cfg.name, flags, &err));
            EXPECT_EQ(e->err->nr_errors, saved_nr_errs);
            EXPECT_NONNULL(find_syntax(syntaxes, path_basename(cfg.name)));
        } else {
            // Check that built-in configs are identical to their source files
            char path[4096];
            xsnprintf(path, sizeof path, "config/%s", cfg.name);
            char *src;
            ssize_t size = read_file(path, &src, 8u << 20);
            EXPECT_MEMEQ(src, size, cfg.text.data, cfg.text.length);
            free(src);
        }
    }

    update_all_syntax_styles(&e->syntaxes, &e->styles);
    e->err->print_to_stderr = false;
}

static void expect_files_equal(TestContext *ctx, const char *path1, const char *path2)
{
    size_t filesize_limit = 1u << 20; // 1MiB
    char *buf1;
    ssize_t size1 = read_file(path1, &buf1, filesize_limit);
    if (size1 < 0) {
        TEST_FAIL("Error reading '%s': %s", path1, strerror(errno));
        return;
    }
    test_pass(ctx);

    char *buf2;
    ssize_t size2 = read_file(path2, &buf2, filesize_limit);
    if (size2 < 0) {
        free(buf1);
        TEST_FAIL("Error reading '%s': %s", path2, strerror(errno));
        return;
    }
    test_pass(ctx);

    if (size1 != size2 || !mem_equal(buf1, buf2, size1)) {
        TEST_FAIL("Files differ: '%s', '%s'", path1, path2);
    } else {
        test_pass(ctx);
    }

    free(buf1);
    free(buf2);
}

static void test_exec_config(TestContext *ctx)
{
    static const char *const outfiles[] = {
        "env.txt",
        "crlf.txt",
        "insert.txt",
        "join.txt",
        "change.txt",
        "thai-utf8.txt",
        "pipe-from.txt",
        "pipe-to.txt",
        "redo1.txt",
        "redo2.txt",
        "replace.txt",
        "shift.txt",
        "repeat.txt",
        "wrap.txt",
        "exec.txt",
        "move.txt",
    };

    // Delete output files left over from previous runs
    unlink("build/test/thai-tis620.txt");
    FOR_EACH_I(i, outfiles) {
        char out[64];
        xsnprintf(out, sizeof out, "build/test/%s", outfiles[i]);
        unlink(out);
    }

    // Execute *.dterc files
    EditorState *e = ctx->userdata;
    FOR_EACH_I(i, builtin_configs) {
        const BuiltinConfig config = builtin_configs[i];
        exec_normal_config(e, config.text);
    }

    // Check that output files have expected contents
    FOR_EACH_I(i, outfiles) {
        char out[64], ref[64];
        xsnprintf(out, sizeof out, "build/test/%s", outfiles[i]);
        xsnprintf(ref, sizeof ref, "test/data/%s", outfiles[i]);
        expect_files_equal(ctx, out, ref);
    }

    if (conversion_supported_by_iconv("UTF-8", "TIS-620")) {
        expect_files_equal(ctx, "build/test/thai-tis620.txt", "test/data/thai-tis620.txt");
    }

    const StringView sv = STRING_VIEW("toggle utf8-bom \\");
    EXPECT_FALSE(e->options.utf8_bom);
    exec_normal_config(e, sv);
    EXPECT_TRUE(e->options.utf8_bom);
    exec_normal_config(e, sv);
    EXPECT_FALSE(e->options.utf8_bom);
}

static void test_detect_indent(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    EXPECT_FALSE(e->options.detect_indent);
    EXPECT_FALSE(e->options.expand_tab);
    EXPECT_EQ(e->options.indent_width, 8);

    static const char cmds[] =
        "option -r '/test/data/detect-indent\\.ini$' detect-indent 2,4,8;"
        "open test/data/detect-indent.ini;";

    EXPECT_TRUE(handle_normal_command(e, cmds, false));
    EXPECT_EQ(e->buffer->options.detect_indent, 1 << 1 | 1 << 3 | 1 << 7);
    EXPECT_TRUE(e->buffer->options.expand_tab);
    EXPECT_EQ(e->buffer->options.indent_width, 2);

    EXPECT_TRUE(handle_normal_command(e, "close", false));
}

static void test_editor_state(TestContext *ctx)
{
    const EditorState *e = ctx->userdata;
    const Buffer *buffer = e->buffer;
    const View *view = e->view;
    const Window *window = e->window;
    const Frame *root_frame = e->root_frame;
    ASSERT_NONNULL(window);
    ASSERT_NONNULL(root_frame);
    ASSERT_NONNULL(buffer);
    ASSERT_NONNULL(view);
    ASSERT_PTREQ(window->view, view);
    ASSERT_PTREQ(window->frame, root_frame);
    ASSERT_PTREQ(window->editor, e);
    ASSERT_PTREQ(view->buffer, buffer);
    ASSERT_PTREQ(view->window, window);
    ASSERT_PTREQ(root_frame->window, window);
    ASSERT_PTREQ(root_frame->parent, NULL);

    ASSERT_EQ(window->views.count, 1);
    ASSERT_EQ(buffer->views.count, 1);
    ASSERT_EQ(e->buffers.count, 1);
    ASSERT_PTREQ(window->views.ptrs[0], view);
    ASSERT_PTREQ(buffer->views.ptrs[0], view);
    ASSERT_PTREQ(window_get_first_view(window), view);
    ASSERT_PTREQ(buffer_get_first_view(buffer), view);
    ASSERT_PTREQ(e->buffers.ptrs[0], buffer);

    ASSERT_NONNULL(buffer->encoding);
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

static void test_handle_normal_command(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    EXPECT_TRUE(handle_normal_command(e, "right; left", false));
    EXPECT_TRUE(handle_normal_command(e, ";left;right;;left;right;;;", false));
    EXPECT_FALSE(handle_normal_command(e, "alias 'err", false));
    EXPECT_FALSE(handle_normal_command(e, "refresh; alias 'x", false));
}

static void test_macro_record(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    CommandMacroState *m = &e->macro;
    EXPECT_PTREQ(e->mode->cmds, &normal_commands);

    EXPECT_EQ(m->macro.count, 0);
    EXPECT_EQ(m->prev_macro.count, 0);
    EXPECT_FALSE(macro_is_recording(m));
    EXPECT_TRUE(macro_record(m));
    EXPECT_TRUE(macro_is_recording(m));

    EXPECT_TRUE(handle_normal_command(e, "open", false));
    EXPECT_TRUE(handle_input(e, 'x'));
    EXPECT_TRUE(handle_input(e, 'y'));
    EXPECT_TRUE(handle_normal_command(e, "bol", true));
    EXPECT_TRUE(handle_input(e, '-'));
    EXPECT_TRUE(handle_input(e, 'z'));
    EXPECT_TRUE(handle_normal_command(e, "eol; right; insert -m .; new-line", true));

    const StringView t1 = STRING_VIEW("test 1\n");
    insert_text(e->view, t1.data, t1.length, true);
    macro_insert_text_hook(m, t1.data, t1.length);

    const StringView t2 = STRING_VIEW("-- test 2");
    insert_text(e->view, t2.data, t2.length, true);
    macro_insert_text_hook(m, t2.data, t2.length);

    EXPECT_TRUE(macro_is_recording(m));
    EXPECT_TRUE(macro_stop(m));
    EXPECT_FALSE(macro_is_recording(m));
    EXPECT_EQ(m->macro.count, 9);
    EXPECT_EQ(m->prev_macro.count, 0);

    static const char cmds[] =
        "save -f build/test/macro-rec.txt;"
        "close -f;"
        "open;"
        "macro play;"
        "save -f build/test/macro-out.txt;"
        "close -f;"
        "show macro;"
        "close -f;";

    EXPECT_TRUE(handle_normal_command(e, cmds, false));
    expect_files_equal(ctx, "build/test/macro-rec.txt", "build/test/macro-out.txt");

    EXPECT_FALSE(macro_is_recording(m));
    EXPECT_TRUE(macro_record(m));
    EXPECT_TRUE(macro_is_recording(m));
    EXPECT_TRUE(handle_input(e, 'x'));
    EXPECT_TRUE(macro_cancel(m));
    EXPECT_FALSE(macro_is_recording(m));
    EXPECT_EQ(m->macro.count, 9);
    EXPECT_EQ(m->prev_macro.count, 0);
}

static const TestEntry tests[] = {
    TEST(test_editor_state),
    TEST(test_handle_normal_command),
    TEST(test_builtin_configs),
    TEST(test_exec_config),
    TEST(test_detect_indent),
    TEST(test_macro_record),
};

const TestGroup config_tests = TEST_GROUP(tests);

DISABLE_WARNING("-Wmissing-prototypes")

void init_headless_mode(TestContext *ctx)
{
    EditorState *e = ctx->userdata;
    ASSERT_NONNULL(e);
    e->terminal.features = TFLAG_8_COLOR;
    e->terminal.width = 80;
    e->terminal.height = 24;
    exec_builtin_rc(e);
    update_all_syntax_styles(&e->syntaxes, &e->styles);
    e->options.lock_files = false;
    e->window = new_window(e);
    e->root_frame = new_root_frame(e->window);
    set_view(window_open_empty_buffer(e->window));
}
