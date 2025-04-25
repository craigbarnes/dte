#include "join.h"
#include "block-iter.h"
#include "change.h"
#include "selection.h"
#include "util/debug.h"
#include "util/unicode.h"

static void join_selection(View *view, const char *delim, size_t delim_len)
{
    size_t count = prepare_selection(view);
    BlockIter bi = view->cursor;
    size_t ws_len = 0;
    size_t join = 0;
    CodePoint ch = 0;
    unselect(view);
    begin_change_chain();

    while (count > 0) {
        if (!ws_len) {
            view->cursor = bi;
        }

        size_t n = block_iter_next_char(&bi, &ch);
        count -= MIN(n, count);
        if (ch == '\n' || ch == '\t' || ch == ' ') {
            join += (ch == '\n');
            ws_len++;
            continue;
        }

        if (join) {
            buffer_replace_bytes(view, ws_len, delim, delim_len);
            // Skip the delimiter we inserted and the char we read last
            block_iter_skip_bytes(&view->cursor, delim_len);
            block_iter_next_char(&view->cursor, &ch);
            bi = view->cursor;
        }

        ws_len = 0;
        join = 0;
    }

    if (join && ch == '\n') {
        // Don't replace last newline at end of selection
        join--;
        ws_len--;
    }

    if (join) {
        size_t ins_len = (ch == '\n') ? 0 : delim_len; // Don't add delim, if at eol
        buffer_replace_bytes(view, ws_len, delim, ins_len);
    }

    end_change_chain(view);
}

void join_lines(View *view, const char *delim, size_t delim_len)
{
    if (view->selection) {
        join_selection(view, delim, delim_len);
        return;
    }

    // Create an iterator and position it at the beginning of the next line
    // (or return early, if there's no next line)
    BlockIter next = view->cursor;
    if (!block_iter_next_line(&next) || block_iter_is_eof(&next)) {
        return;
    }

    // Create a second iterator and position it at the end of the current line
    BlockIter eol = next;
    CodePoint u;
    size_t nbytes = block_iter_prev_char(&eol, &u);
    BUG_ON(nbytes != 1);
    BUG_ON(u != '\n');

    // Skip over trailing whitespace at the end of the current line
    size_t del_count = 1 + block_iter_skip_blanks_bwd(&eol);

    // Skip over leading whitespace at the start of the next line
    del_count += block_iter_skip_blanks_fwd(&next);

    // Move the cursor to the join position
    view->cursor = eol;

    if (block_iter_is_bol(&next)) {
        // If the next line is empty (or whitespace only) just discard
        // it, by deleting the newline and any whitespace
        buffer_delete_bytes(view, del_count);
    } else {
        // Otherwise, join the current and next lines together, by
        // replacing the newline/whitespace with the delimiter string
        buffer_replace_bytes(view, del_count, delim, delim_len);
    }
}
