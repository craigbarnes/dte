#include <stdlib.h>
#include <string.h>
#include "delete.h"
#include "buffer.h"
#include "change.h"
#include "indent.h"
#include "options.h"
#include "selection.h"
#include "util/string-view.h"

void delete_ch(View *view)
{
    size_t size = 0;
    if (view->selection) {
        size = prepare_selection(view);
        unselect(view);
    } else {
        const LocalOptions *options = &view->buffer->options;
        begin_change(CHANGE_MERGE_DELETE);
        if (options->emulate_tab) {
            size = get_indent_level_bytes_right(options, &view->cursor);
        }
        if (size == 0) {
            BlockIter bi = view->cursor;
            size = block_iter_next_column(&bi);
        }
    }
    buffer_delete_bytes(view, size);
}

void erase(View *view)
{
    size_t size = 0;
    if (view->selection) {
        size = prepare_selection(view);
        unselect(view);
    } else {
        const LocalOptions *options = &view->buffer->options;
        begin_change(CHANGE_MERGE_ERASE);
        if (options->emulate_tab) {
            size = get_indent_level_bytes_left(options, &view->cursor);
            block_iter_back_bytes(&view->cursor, size);
        }
        if (size == 0) {
            CodePoint u;
            size = block_iter_prev_char(&view->cursor, &u);
        }
    }
    buffer_erase_bytes(view, size);
}

void clear_lines(View *view, bool auto_indent)
{
    String indent = STRING_INIT;
    if (auto_indent) {
        BlockIter bi = view->cursor;
        if (block_iter_prev_line(&bi) && block_iter_find_non_empty_line_bwd(&bi)) {
            StringView line = block_iter_get_line(&bi);
            indent = get_indent_for_next_line(&view->buffer->options, line);
        }
    }

    size_t del_count = 0;
    if (view->selection) {
        view->selection = SELECT_LINES;
        del_count = prepare_selection(view);
        unselect(view);
        // Don't delete last newline
        if (del_count) {
            del_count--;
        }
    } else {
        block_iter_eol(&view->cursor);
        del_count = block_iter_bol(&view->cursor);
    }

    if (!indent.len && !del_count) {
        return;
    }

    buffer_replace_bytes(view, del_count, indent.buffer, indent.len);
    block_iter_skip_bytes(&view->cursor, indent.len);
    string_free(&indent);
}
