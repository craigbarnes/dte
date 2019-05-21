#include "cmdline.h"
#include "editor.h"
#include "history.h"
#include "mode.h"
#include "search.h"

static void search_mode_keypress(KeyCode key)
{
    switch (key) {
    case KEY_ENTER:
        if (editor.cmdline.buf.len > 0) {
            const char *str = string_borrow_cstring(&editor.cmdline.buf);
            search_set_regexp(str);
            search_next();
            history_add(&editor.search_history, str, search_history_size);
        } else {
            search_next();
        }
        cmdline_clear(&editor.cmdline);
        set_input_mode(INPUT_NORMAL);
        return;
    case MOD_META | 'c':
        editor.options.case_sensitive_search = (editor.options.case_sensitive_search + 1) % 3;
        return;
    case MOD_META | 'r':
        search_set_direction(current_search_direction() ^ 1);
        return;
    case '\t':
        return;
    }

    switch (cmdline_handle_key(&editor.cmdline, &editor.search_history, key)) {
    case CMDLINE_CANCEL:
        set_input_mode(INPUT_NORMAL);
        return;
    case CMDLINE_UNKNOWN_KEY:
    case CMDLINE_KEY_HANDLED:
        return;
    }
}

const EditorModeOps search_mode_ops = {
    .keypress = search_mode_keypress,
    .update = normal_update,
};
