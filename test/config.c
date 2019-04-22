#include "test.h"
#include "../src/common.h"
#include "../src/config.h"
#include "../src/debug.h"
#include "../src/editor.h"
#include "../src/frame.h"
#include "../src/terminal/terminal.h"
#include "../src/util/string-view.h"
#include "../src/window.h"
#include "../build/test/data.h"

static const char extra_rc[] =
    "set lock-files false\n"
    // Regression test for unquoted variables in rc files
    "bind M-p \"insert \"$WORD\n"
    "bind M-p \"insert \"$FILE\n"
;

static void no_op(void) {}

void init_headless_mode(void)
{
    memzero(&terminal.control_codes);
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

void test_exec_config(void)
{
    BUG_ON(!window);
    FOR_EACH_I(i, builtin_configs) {
        const BuiltinConfig config = builtin_configs[i];
        exec_config(commands, config.text.data, config.text.length);
    }
}
