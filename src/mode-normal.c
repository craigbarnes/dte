#include "bind.h"
#include "change.h"
#include "edit.h"
#include "editor.h"
#include "mode.h"
#include "terminal/input.h"
#include "util/unicode.h"
#include "view.h"
#include "window.h"

static void insert_paste(void)
{
    size_t size;
    char *text = term_read_paste(&size);

    // Because this is not a command (see run_command()) you have to
    // call begin_change() to avoid merging this change into previous
    begin_change(CHANGE_MERGE_NONE);
    insert_text(text, size);
    end_change();

    free(text);
}

static void normal_mode_keypress(KeyCode key)
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
    case KEY_PASTE:
        insert_paste();
        return;
    }

    if (u_is_unicode(key)) {
        insert_ch(key);
    } else {
        handle_binding(key);
    }
}

const EditorModeOps normal_mode_ops = {
    .keypress = normal_mode_keypress,
    .update = normal_update,
};
