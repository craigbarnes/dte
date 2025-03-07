#include "copy.h"
#include "block-iter.h"
#include "change.h"
#include "insert.h"
#include "move.h"
#include "selection.h"

void paste(Clipboard *clip, View *view, PasteLinesType type, bool move_after)
{
    if (clip->len == 0) {
        return;
    }

    BUG_ON(!clip->buf);
    if (!clip->is_lines || type == PASTE_LINES_INLINE) {
        insert_text(view, clip->buf, clip->len, move_after);
        return;
    }

    size_t del_count = 0;
    if (view->selection) {
        del_count = prepare_selection(view);
        unselect(view);
    }

    const long x = view_get_preferred_x(view);
    if (!del_count) {
        if (type == PASTE_LINES_BELOW_CURSOR) {
            block_iter_eat_line(&view->cursor);
        } else {
            BUG_ON(type != PASTE_LINES_ABOVE_CURSOR);
            block_iter_bol(&view->cursor);
        }
    }

    buffer_replace_bytes(view, del_count, clip->buf, clip->len);

    if (move_after) {
        block_iter_skip_bytes(&view->cursor, clip->len);
    } else {
        // Try to keep cursor column
        move_to_preferred_x(view, x);
    }

    // New preferred_x
    view_reset_preferred_x(view);
}
