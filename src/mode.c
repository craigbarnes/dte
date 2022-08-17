#include "mode.h"
#include "bind.h"
#include "buffer.h"
#include "change.h"
#include "cmdline.h"
#include "command/macro.h"
#include "completion.h"
#include "history.h"
#include "misc.h"
#include "search.h"
#include "shift.h"
#include "terminal/input.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/unicode.h"
#include "view.h"

static void normal_mode_keypress(EditorState *e, KeyCode key)
{
    View *view = e->view;
    switch (key) {
    case KEY_TAB:
        if (view->selection == SELECT_LINES) {
            shift_lines(view, 1);
            return;
        }
        break;
    case MOD_SHIFT | KEY_TAB:
        if (view->selection == SELECT_LINES) {
            shift_lines(view, -1);
            return;
        }
        break;
    case KEY_BRACKETED_PASTE:
    case KEY_DETECTED_PASTE: {
        String str = term_read_paste(&e->terminal.ibuf, key == KEY_BRACKETED_PASTE);
        begin_change(CHANGE_MERGE_NONE);
        insert_text(view, str.buffer, str.len, true);
        end_change();
        macro_insert_text_hook(&e->macro, str.buffer, str.len);
        string_free(&str);
        return;
        }
    }

    if (u_is_unicode(key)) {
        insert_ch(view, key);
        macro_insert_char_hook(&e->macro, key);
    } else {
        handle_binding(&e->bindings[INPUT_NORMAL], key);
    }
}

static void cmdline_insert_paste(CommandLine *c, TermInputBuffer *input, bool bracketed)
{
    String str = term_read_paste(input, bracketed);
    string_replace_byte(&str, '\n', ' ');
    string_insert_buf(&c->buf, c->pos, str.buffer, str.len);
    c->pos += str.len;
    c->search_pos = NULL;
    string_free(&str);
}

void handle_input(EditorState *e, KeyCode key)
{
    InputMode mode = e->input_mode;
    if (mode == INPUT_NORMAL) {
        normal_mode_keypress(e, key);
        return;
    }

    BUG_ON(!(mode == INPUT_COMMAND || mode == INPUT_SEARCH));
    CommandLine *c = &e->cmdline;
    if (u_is_unicode(key) && key != KEY_TAB && key != KEY_ENTER) {
        c->pos += string_insert_ch(&c->buf, c->pos, key);
    } else if (key == KEY_DETECTED_PASTE || key == KEY_BRACKETED_PASTE) {
        cmdline_insert_paste(c, &e->terminal.ibuf, key == KEY_BRACKETED_PASTE);
    } else {
        handle_binding(&e->bindings[mode], key);
        return;
    }

    if (mode == INPUT_COMMAND) {
        reset_completion(c);
    }
}
