#include "mode.h"
#include "bind.h"
#include "change.h"
#include "cmdline.h"
#include "command.h"
#include "completion.h"
#include "edit.h"
#include "editor.h"
#include "error.h"
#include "history.h"
#include "search.h"
#include "terminal/input.h"
#include "util/unicode.h"
#include "view.h"
#include "window.h"

void normal_mode_keypress(KeyCode key)
{
    switch (key) {
    case '\t':
        if (view->selection == SELECT_LINES) {
            shift_lines(1);
            return;
        }
        break;
    case MOD_SHIFT | '\t':
        if (view->selection == SELECT_LINES) {
            shift_lines(-1);
            return;
        }
        break;
    case KEY_PASTE: {
        size_t size;
        char *text = term_read_paste(&size);
        begin_change(CHANGE_MERGE_NONE);
        insert_text(text, size);
        end_change();
        free(text);
        return;
        }
    }

    if (u_is_unicode(key)) {
        insert_ch(key);
    } else {
        handle_binding(key);
    }
}

void command_mode_keypress(KeyCode key)
{
    switch (key) {
    case KEY_ENTER:
        reset_completion();
        set_input_mode(INPUT_NORMAL);
        const char *str = string_borrow_cstring(&editor.cmdline.buf);
        cmdline_clear(&editor.cmdline);
        if (str[0] != ' ') {
            // This is done before handle_command() because "command [text]"
            // can modify the contents of the command-line
            history_add(&editor.command_history, str, command_history_size);
        }
        handle_command(&commands, str);
        return;
    case '\t':
        complete_command_next();
        return;
    case MOD_SHIFT | '\t':
        complete_command_prev();
        return;
    }

    switch (cmdline_handle_key(&editor.cmdline, &editor.command_history, key)) {
    case CMDLINE_CANCEL:
        set_input_mode(INPUT_NORMAL);
        // Fallthrough
    case CMDLINE_KEY_HANDLED:
        reset_completion();
        // Fallthrough
    case CMDLINE_UNKNOWN_KEY:
        return;
    }
}

void search_mode_keypress(KeyCode key)
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
