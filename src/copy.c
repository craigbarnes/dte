#include <stdlib.h>
#include "copy.h"
#include "block-iter.h"
#include "change.h"
#include "misc.h"
#include "move.h"
#include "selection.h"

void record_copy(Clipboard *clip, char *buf, size_t len, bool is_lines)
{
    if (clip->buf) {
        free(clip->buf);
    }
    clip->buf = buf;
    clip->len = len;
    clip->is_lines = is_lines;
}

void copy(Clipboard *clip, View *view, size_t len, bool is_lines)
{
    if (len) {
        char *buf = block_iter_get_bytes(&view->cursor, len);
        record_copy(clip, buf, len, is_lines);
    }
}

void cut(Clipboard *clip, View *view, size_t len, bool is_lines)
{
    if (len) {
        char *buf = block_iter_get_bytes(&view->cursor, len);
        record_copy(clip, buf, len, is_lines);
        buffer_delete_bytes(view, len);
    }
}

void paste(Clipboard *clip, View *view, bool at_cursor)
{
    if (!clip->buf) {
        return;
    }

    size_t del_count = 0;
    if (view->selection) {
        del_count = prepare_selection(view);
        unselect(view);
    }

    if (clip->is_lines && !at_cursor) {
        const long x = view_get_preferred_x(view);
        if (!del_count) {
            block_iter_eat_line(&view->cursor);
        }
        buffer_replace_bytes(view, del_count, clip->buf, clip->len);

        // Try to keep cursor column
        move_to_preferred_x(view, x);
        // New preferred_x
        view_reset_preferred_x(view);
    } else {
        buffer_replace_bytes(view, del_count, clip->buf, clip->len);
    }
}
