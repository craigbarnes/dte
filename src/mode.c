#include <stddef.h>
#include "mode.h"
#include "bind.h"
#include "change.h"
#include "cmdline.h"
#include "command/macro.h"
#include "completion.h"
#include "editor.h"
#include "error.h"
#include "misc.h"
#include "shift.h"
#include "terminal/paste.h"
#include "util/debug.h"
#include "util/unicode.h"
#include "util/xmalloc.h"
#include "view.h"

ModeHandler *new_mode(HashMap *modes, char *name, const CommandSet *cmds)
{
    ModeHandler *mode = xnew0(ModeHandler, 1);
    mode->name = name;
    mode->cmds = cmds;
    mode->insert_text_for_unicode_range = true;
    return hashmap_insert(modes, name, mode);
}

static bool insert_paste(EditorState *e, const ModeHandler *handler, bool bracketed)
{
    String str = term_read_paste(&e->terminal.ibuf, bracketed);
    if (handler->cmds == &normal_commands) {
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

static bool handle_input_single(EditorState *e, const ModeHandler *handler, KeyCode key)
{
    if (key == KEY_DETECTED_PASTE || key == KEY_BRACKETED_PASTE) {
        return insert_paste(e, handler, key == KEY_BRACKETED_PASTE);
    }

    const CommandSet *cmds = handler->cmds;
    if (handler->insert_text_for_unicode_range) {
        if (cmds == &normal_commands) {
            View *view = e->view;
            KeyCode shift = key & MOD_SHIFT;
            if ((key & ~shift) == KEY_TAB && view->selection == SELECT_LINES) {
                // In line selections, Tab/S-Tab behave like `shift -- 1/-1`
                shift_lines(view, shift ? -1 : 1);
                return true;
            }
            if (u_is_unicode(key)) {
                insert_ch(e->view, key);
                macro_insert_char_hook(&e->macro, key);
                return true;
            }
        } else {
            BUG_ON(cmds != &cmd_mode_commands && cmds != &search_mode_commands);
            if (u_is_unicode(key) && (key != KEY_TAB && key != KEY_ENTER)) {
                CommandLine *c = &e->cmdline;
                c->pos += string_insert_codepoint(&c->buf, c->pos, key);
                if (cmds == &cmd_mode_commands) {
                    reset_completion(c);
                }
                return true;
            }
        }
    }

    return handle_binding(e, handler, key);
}

// NOLINTNEXTLINE(misc-no-recursion)
static bool handle_input_recursive (
    EditorState *e,
    const ModeHandler *handler,
    KeyCode key
) {
    if (handle_input_single(e, handler, key)) {
        return true;
    }

    const PointerArray *ftmodes = &handler->fallthrough_modes;
    for (size_t i = 0, n = ftmodes->count; i < n; i++) {
        const ModeHandler *h = ftmodes->ptrs[i];
        if (handle_input_recursive(e, h, key)) {
            return true;
        }
    }

    return false;
}

bool handle_input(EditorState *e, KeyCode key)
{
    return handle_input_recursive(e, e->mode, key);
}
