#include <stddef.h>
#include "mode.h"
#include "bind.h"
#include "change.h"
#include "cmdline.h"
#include "command/macro.h"
#include "completion.h"
#include "editor.h"
#include "insert.h"
#include "shift.h"
#include "terminal/paste.h"
#include "util/debug.h"
#include "util/unicode.h"
#include "util/xmalloc.h"
#include "view.h"

ModeHandler *new_mode(HashMap *modes, char *name, const CommandSet *cmds)
{
    ModeHandler *mode = xcalloc1(sizeof(*mode));
    mode->name = name;
    mode->cmds = cmds;
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

static bool handle_input_single (
    EditorState *e,
    const ModeHandler *handler,
    KeyCode key,
    ModeHandlerFlags inherited_flags
) {
    if (key == KEYCODE_DETECTED_PASTE || key == KEYCODE_BRACKETED_PASTE) {
        return insert_paste(e, handler, key == KEYCODE_BRACKETED_PASTE);
    }

    ModeHandlerFlags noinsert_flags = MHF_NO_TEXT_INSERTION | MHF_NO_TEXT_INSERTION_RECURSIVE;
    ModeHandlerFlags flag_union = handler->flags | inherited_flags;
    bool insert_unicode_range = !(flag_union & noinsert_flags);

    if (insert_unicode_range) {
        const CommandSet *cmds = handler->cmds;
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
                    maybe_reset_completion(c);
                }
                c->search_pos = NULL;
                return true;
            }
        }
    }

    return handle_binding(e, handler, key);
}

// Recursion is bounded by the depth of fallthrough modes, which is
// typically not more than 5 or so
// NOLINTNEXTLINE(misc-no-recursion)
static bool handle_input_recursive (
    EditorState *e,
    const ModeHandler *handler,
    KeyCode key,
    ModeHandlerFlags inherited_flags
) {
    if (handle_input_single(e, handler, key, inherited_flags)) {
        return true;
    }

    const PointerArray *ftmodes = &handler->fallthrough_modes;
    inherited_flags |= (handler->flags & MHF_NO_TEXT_INSERTION_RECURSIVE);

    for (size_t i = 0, n = ftmodes->count; i < n; i++) {
        const ModeHandler *h = ftmodes->ptrs[i];
        if (handle_input_recursive(e, h, key, inherited_flags)) {
            return true;
        }
    }

    return false;
}

void string_append_def_mode(String *buf, const ModeHandler *mode)
{
    string_append_literal(buf, "def-mode ");
    if (mode->flags & MHF_NO_TEXT_INSERTION_RECURSIVE) {
        string_append_literal(buf, "-U ");
    } else if (mode->flags & MHF_NO_TEXT_INSERTION) {
        string_append_literal(buf, "-u ");
    }

    string_append_cstring(buf, mode->name);

    const PointerArray *ftmodes = &mode->fallthrough_modes;
    for (size_t i = 0, n = ftmodes->count; i < n; i++) {
        const ModeHandler *ftmode = ftmodes->ptrs[i];
        string_append_byte(buf, ' ');
        string_append_cstring(buf, ftmode->name);
    }
}

bool handle_input(EditorState *e, KeyCode key)
{
    return handle_input_recursive(e, e->mode, key, 0);
}

void collect_modes(const HashMap *modes, PointerArray *a, const char *prefix)
{
    collect_hashmap_keys(modes, a, prefix);
}
