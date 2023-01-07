#include "mode.h"
#include "bind.h"
#include "change.h"
#include "cmdline.h"
#include "command/macro.h"
#include "completion.h"
#include "misc.h"
#include "shift.h"
#include "terminal/input.h"
#include "util/debug.h"
#include "util/unicode.h"
#include "view.h"

static bool normal_mode_keypress(EditorState *e, KeyCode key)
{
    View *view = e->view;
    KeyCode shift = key & MOD_SHIFT;
    if ((key & ~shift) == KEY_TAB && view->selection == SELECT_LINES) {
        // In line selections, Tab/S-Tab behave like `shift -- 1/-1`
        shift_lines(view, shift ? -1 : 1);
        return true;
    }

    if (u_is_unicode(key)) {
        insert_ch(view, key);
        macro_insert_char_hook(&e->macro, key);
        return true;
    }

    return handle_binding(e, INPUT_NORMAL, key);
}

static bool insert_paste(EditorState *e, bool bracketed)
{
    String str = term_read_paste(&e->terminal.ibuf, bracketed);
    if (e->input_mode == INPUT_NORMAL) {
        begin_change(CHANGE_MERGE_NONE);
        insert_text(e->view, str.buffer, str.len, true);
        end_change();
        macro_insert_text_hook(&e->macro, str.buffer, str.len);
    } else {
        CommandLine *c = &e->cmdline;
        string_replace_byte(&str, '\n', ' ');
        string_insert_buf(&c->buf, c->pos, str.buffer, str.len);
        c->pos += str.len;
        c->search_pos = NULL;
    }
    string_free(&str);
    return true;
}

bool handle_input(EditorState *e, KeyCode key)
{
    if (key == KEY_DETECTED_PASTE || key == KEY_BRACKETED_PASTE) {
        return insert_paste(e, key == KEY_BRACKETED_PASTE);
    }

    InputMode mode = e->input_mode;
    if (mode == INPUT_NORMAL) {
        return normal_mode_keypress(e, key);
    }

    BUG_ON(!(mode == INPUT_COMMAND || mode == INPUT_SEARCH));
    if (!u_is_unicode(key) || key == KEY_TAB || key == KEY_ENTER) {
        return handle_binding(e, mode, key);
    }

    CommandLine *c = &e->cmdline;
    c->pos += string_insert_codepoint(&c->buf, c->pos, key);
    if (mode == INPUT_COMMAND) {
        reset_completion(c);
    }
    return true;
}
