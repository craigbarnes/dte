#include "mode.h"
#include "bind.h"
#include "buffer.h"
#include "change.h"
#include "cmdline.h"
#include "command/macro.h"
#include "commands.h"
#include "completion.h"
#include "edit.h"
#include "editor.h"
#include "history.h"
#include "search.h"
#include "terminal/input.h"
#include "util/debug.h"
#include "util/unicode.h"
#include "view.h"

static void normal_mode_keypress(KeyCode key)
{
    switch (key) {
    case KEY_TAB:
        if (view->selection == SELECT_LINES) {
            shift_lines(1);
            return;
        }
        break;
    case MOD_SHIFT | KEY_TAB:
        if (view->selection == SELECT_LINES) {
            shift_lines(-1);
            return;
        }
        break;
    case KEY_PASTE: {
        size_t size;
        char *text = term_read_paste(&size);
        begin_change(CHANGE_MERGE_NONE);
        insert_text(text, size, true);
        end_change();
        macro_insert_text_hook(text, size);
        free(text);
        return;
        }
    }

    if (u_is_unicode(key)) {
        insert_ch(key);
        macro_insert_char_hook(key);
    } else {
        handle_binding(key);
    }
}

static void command_mode_keypress(KeyCode key)
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
            history_add(&editor.command_history, str);
        }
        handle_command(&commands, str, true);
        return;
    case KEY_TAB:
        complete_command_next();
        return;
    case MOD_SHIFT | KEY_TAB:
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

static void search_mode_keypress(KeyCode key)
{
    switch (key) {
    case MOD_META | KEY_ENTER:
        if (editor.cmdline.buf.len == 0) {
            return;
        } else {
            // Escape the regex; to match as plain text
            char *original = string_clone_cstring(&editor.cmdline.buf);
            size_t len = editor.cmdline.buf.len;
            string_clear(&editor.cmdline.buf);
            for (size_t i = 0; i < len; i++) {
                char ch = original[i];
                if (is_regex_special_char(ch)) {
                    string_append_byte(&editor.cmdline.buf, '\\');
                }
                string_append_byte(&editor.cmdline.buf, ch);
            }
            free(original);
        }
        // Fallthrough
    case KEY_ENTER: {
        const char *args[3] = {NULL, NULL, NULL};
        if (editor.cmdline.buf.len > 0) {
            const char *str = string_borrow_cstring(&editor.cmdline.buf);
            search_set_regexp(str);
            history_add(&editor.search_history, str);
            if (unlikely(str[0] == '-')) {
                args[0] = "--";
                args[1] = str;
            } else {
                args[0] = str;
            }
        } else {
            args[0] = "-n";
        }
        search_next();
        macro_command_hook("search", (char**)args);
        cmdline_clear(&editor.cmdline);
        set_input_mode(INPUT_NORMAL);
        return;
    }
    case MOD_META | 'c':
        editor.options.case_sensitive_search = (editor.options.case_sensitive_search + 1) % 3;
        return;
    case MOD_META | 'r':
        toggle_search_direction();
        return;
    case KEY_TAB:
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

void handle_input(KeyCode key)
{
    switch (editor.input_mode) {
    case INPUT_NORMAL:
        normal_mode_keypress(key);
        break;
    case INPUT_COMMAND:
        command_mode_keypress(key);
        break;
    case INPUT_SEARCH:
        search_mode_keypress(key);
        break;
    default:
        BUG("unhandled input mode");
    }
}
