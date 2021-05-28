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
        handle_binding(INPUT_NORMAL, key);
    }
}

static void cmdline_insert_paste(CommandLine *c)
{
    size_t size;
    char *text = term_read_paste(&size);
    for (size_t i = 0; i < size; i++) {
        if (text[i] == '\n') {
            text[i] = ' ';
        }
    }
    string_insert_buf(&c->buf, c->pos, text, size);
    c->pos += size;
    free(text);
}

static void command_mode_keypress(KeyCode key)
{
    CommandLine *c = &editor.cmdline;
    switch (key) {
    case KEY_ENTER:
        reset_completion();
        set_input_mode(INPUT_NORMAL);
        const char *str = string_borrow_cstring(&c->buf);
        cmdline_clear(c);
        if (str[0] != ' ') {
            // This is done before handle_command() because "command [text]"
            // can modify the contents of the command-line
            history_add(&editor.command_history, str);
        }
        handle_command(&normal_commands, str, true);
        return;
    case KEY_TAB:
        complete_command_next();
        return;
    case MOD_SHIFT | KEY_TAB:
        complete_command_prev();
        return;
    case KEY_PASTE:
        cmdline_insert_paste(c);
        c->search_pos = NULL;
        reset_completion();
        return;
    }

    if (u_is_unicode(key)) {
        c->pos += string_insert_ch(&c->buf, c->pos, key);
        reset_completion();
    } else {
        handle_binding(INPUT_COMMAND, key);
    }
}

static void search_mode_keypress(KeyCode key)
{
    CommandLine *c = &editor.cmdline;
    switch (key) {
    case MOD_META | KEY_ENTER:
        if (c->buf.len == 0) {
            return;
        } else {
            // Escape the regex; to match as plain text
            char *original = string_clone_cstring(&c->buf);
            size_t len = c->buf.len;
            string_clear(&c->buf);
            for (size_t i = 0; i < len; i++) {
                char ch = original[i];
                if (is_regex_special_char(ch)) {
                    string_append_byte(&c->buf, '\\');
                }
                string_append_byte(&c->buf, ch);
            }
            free(original);
        }
        // Fallthrough
    case KEY_ENTER: {
        const char *args[3] = {NULL, NULL, NULL};
        if (c->buf.len > 0) {
            const char *str = string_borrow_cstring(&c->buf);
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
        cmdline_clear(c);
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
    case KEY_PASTE:
        cmdline_insert_paste(c);
        c->search_pos = NULL;
        return;
    }

    if (u_is_unicode(key)) {
        c->pos += string_insert_ch(&c->buf, c->pos, key);
    } else {
        if (!handle_binding(INPUT_SEARCH, key)) {
            handle_binding(INPUT_COMMAND, key);
        }
    }
}

void handle_input(KeyCode key)
{
    switch (editor.input_mode) {
    case INPUT_NORMAL:
        normal_mode_keypress(key);
        return;
    case INPUT_COMMAND:
        command_mode_keypress(key);
        return;
    case INPUT_SEARCH:
        search_mode_keypress(key);
        return;
    }

    BUG("unhandled input mode");
}
