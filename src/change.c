#include <stdlib.h>
#include <string.h>
#include "change.h"
#include "buffer.h"
#include "edit.h"
#include "error.h"
#include "util/debug.h"
#include "util/xmalloc.h"

static ChangeMergeEnum change_merge;
static ChangeMergeEnum prev_change_merge;

static Change *alloc_change(void)
{
    return xcalloc(sizeof(Change));
}

static void add_change(Buffer *buffer, Change *change)
{
    Change *head = buffer->cur_change;
    change->next = head;
    xrenew(head->prev, head->nr_prev + 1);
    head->prev[head->nr_prev++] = change;
    buffer->cur_change = change;
}

// This doesn't need to be local to buffer because commands are atomic
static Change *change_barrier;

static bool is_change_chain_barrier(const Change *change)
{
    return !change->ins_count && !change->del_count;
}

static Change *new_change(Buffer *buffer)
{
    if (change_barrier) {
        /*
         * We are recording series of changes (:replace for example)
         * and now we have just made the first change so we have to
         * mark beginning of the chain.
         *
         * We could have done this before when starting the change
         * chain but then we may have ended up with an empty chain.
         * We don't want to record empty changes ever.
         */
        add_change(buffer, change_barrier);
        change_barrier = NULL;
    }

    Change *change = alloc_change();
    add_change(buffer, change);
    return change;
}

static size_t buffer_offset(const View *view)
{
    return block_iter_get_offset(&view->cursor);
}

static void record_insert(View *view, size_t len)
{
    Change *change = view->buffer->cur_change;
    BUG_ON(!len);
    if (
        change_merge == prev_change_merge
        && change_merge == CHANGE_MERGE_INSERT
    ) {
        BUG_ON(change->del_count);
        change->ins_count += len;
        return;
    }

    change = new_change(view->buffer);
    change->offset = buffer_offset(view);
    change->ins_count = len;
}

static void record_delete(View *view, char *buf, size_t len, bool move_after)
{
    BUG_ON(!len);
    BUG_ON(!buf);

    Change *change = view->buffer->cur_change;
    if (change_merge == prev_change_merge) {
        if (change_merge == CHANGE_MERGE_DELETE) {
            xrenew(change->buf, change->del_count + len);
            memcpy(change->buf + change->del_count, buf, len);
            change->del_count += len;
            free(buf);
            return;
        }
        if (change_merge == CHANGE_MERGE_ERASE) {
            xrenew(buf, len + change->del_count);
            memcpy(buf + len, change->buf, change->del_count);
            change->del_count += len;
            free(change->buf);
            change->buf = buf;
            change->offset -= len;
            return;
        }
    }

    change = new_change(view->buffer);
    change->offset = buffer_offset(view);
    change->del_count = len;
    change->move_after = move_after;
    change->buf = buf;
}

static void record_replace(View *view, char *deleted, size_t del_count, size_t ins_count)
{
    BUG_ON(del_count && !deleted);
    BUG_ON(!del_count && deleted);
    BUG_ON(!del_count && !ins_count);

    Change *change = new_change(view->buffer);
    change->offset = buffer_offset(view);
    change->ins_count = ins_count;
    change->del_count = del_count;
    change->buf = deleted;
}

void begin_change(ChangeMergeEnum m)
{
    change_merge = m;
}

void end_change(void)
{
    prev_change_merge = change_merge;
}

void begin_change_chain(void)
{
    BUG_ON(change_barrier);

    // Allocate change chain barrier but add it to the change tree only if
    // there will be any real changes
    change_barrier = alloc_change();
    change_merge = CHANGE_MERGE_NONE;
}

void end_change_chain(View *view)
{
    if (change_barrier) {
        // There were no changes in this change chain
        free(change_barrier);
        change_barrier = NULL;
    } else {
        // There were some changes; add end of chain marker
        add_change(view->buffer, alloc_change());
    }
}

static void fix_cursors(const View *view, size_t offset, size_t del, size_t ins)
{
    const Buffer *buffer = view->buffer;
    for (size_t i = 0, n = buffer->views.count; i < n; i++) {
        View *v = buffer->views.ptrs[i];
        if (v != view && offset < v->saved_cursor_offset) {
            if (offset + del <= v->saved_cursor_offset) {
                v->saved_cursor_offset -= del;
                v->saved_cursor_offset += ins;
            } else {
                v->saved_cursor_offset = offset;
            }
        }
    }
}

static void reverse_change(View *view, Change *change)
{
    if (view->buffer->views.count > 1) {
        fix_cursors(view, change->offset, change->ins_count, change->del_count);
    }

    block_iter_goto_offset(&view->cursor, change->offset);
    if (!change->ins_count) {
        // Convert delete to insert
        do_insert(view, change->buf, change->del_count);
        if (change->move_after) {
            block_iter_skip_bytes(&view->cursor, change->del_count);
        }
        change->ins_count = change->del_count;
        change->del_count = 0;
        free(change->buf);
        change->buf = NULL;
    } else if (change->del_count) {
        // Reverse replace
        size_t del_count = change->ins_count;
        size_t ins_count = change->del_count;
        char *buf = do_replace(view, del_count, change->buf, ins_count);
        free(change->buf);
        change->buf = buf;
        change->ins_count = ins_count;
        change->del_count = del_count;
    } else {
        // Convert insert to delete
        change->buf = do_delete(view, change->ins_count, true);
        change->del_count = change->ins_count;
        change->ins_count = 0;
    }
}

bool undo(View *view)
{
    Change *change = view->buffer->cur_change;
    view_reset_preferred_x(view);
    if (!change->next) {
        return false;
    }

    if (is_change_chain_barrier(change)) {
        unsigned long count = 0;
        while (1) {
            change = change->next;
            if (is_change_chain_barrier(change)) {
                break;
            }
            reverse_change(view, change);
            count++;
        }
        if (count > 1) {
            info_msg("Undid %lu changes", count);
        }
    } else {
        reverse_change(view, change);
    }

    view->buffer->cur_change = change->next;
    return true;
}

bool redo(View *view, unsigned long change_id)
{
    Change *change = view->buffer->cur_change;
    view_reset_preferred_x(view);
    if (!change->prev) {
        // Don't complain if change_id is 0
        if (change_id) {
            error_msg("Nothing to redo");
        }
        return false;
    }

    const unsigned long nr_prev = change->nr_prev;
    BUG_ON(nr_prev == 0);
    if (change_id == 0) {
        // Default to newest change
        change_id = nr_prev - 1;
        if (nr_prev > 1) {
            unsigned long i = change_id + 1;
            info_msg("Redoing newest (%lu) of %lu possible changes", i, nr_prev);
        }
    } else {
        if (--change_id >= nr_prev) {
            if (nr_prev == 1) {
                return error_msg("There is only 1 possible change to redo");
            }
            return error_msg("There are only %lu possible changes to redo", nr_prev);
        }
    }

    change = change->prev[change_id];
    if (is_change_chain_barrier(change)) {
        unsigned long count = 0;
        while (1) {
            change = change->prev[change->nr_prev - 1];
            if (is_change_chain_barrier(change)) {
                break;
            }
            reverse_change(view, change);
            count++;
        }
        if (count > 1) {
            info_msg("Redid %lu changes", count);
        }
    } else {
        reverse_change(view, change);
    }

    view->buffer->cur_change = change;
    return true;
}

void free_changes(Change *c)
{
top:
    while (c->nr_prev) {
        c = c->prev[c->nr_prev - 1];
    }

    // c is leaf now
    while (c->next) {
        Change *next = c->next;
        free(c->buf);
        free(c);

        c = next;
        if (--c->nr_prev) {
            goto top;
        }

        // We have become leaf
        free(c->prev);
    }
}

void buffer_insert_bytes(View *view, const char *buf, const size_t len)
{
    view_reset_preferred_x(view);
    if (len == 0) {
        return;
    }

    size_t rec_len = len;
    if (buf[len - 1] != '\n' && block_iter_is_eof(&view->cursor)) {
        // Force newline at EOF
        do_insert(view, "\n", 1);
        rec_len++;
    }

    do_insert(view, buf, len);
    record_insert(view, rec_len);

    if (view->buffer->views.count > 1) {
        fix_cursors(view, block_iter_get_offset(&view->cursor), len, 0);
    }
}

static bool would_delete_last_bytes(const View *view, size_t count)
{
    const Block *blk = view->cursor.blk;
    size_t offset = view->cursor.offset;
    while (1) {
        size_t avail = blk->size - offset;
        if (avail > count) {
            return false;
        }

        if (blk->node.next == view->cursor.head) {
            return true;
        }

        count -= avail;
        blk = BLOCK(blk->node.next);
        offset = 0;
    }
}

static void buffer_delete_bytes_internal(View *view, size_t len, bool move_after)
{
    view_reset_preferred_x(view);
    if (len == 0) {
        return;
    }

    // Check if all newlines from EOF would be deleted
    if (would_delete_last_bytes(view, len)) {
        BlockIter bi = view->cursor;
        CodePoint u;
        if (block_iter_prev_char(&bi, &u) && u != '\n') {
            // No newline before cursor
            if (--len == 0) {
                begin_change(CHANGE_MERGE_NONE);
                return;
            }
        }
    }
    record_delete(view, do_delete(view, len, true), len, move_after);

    if (view->buffer->views.count > 1) {
        fix_cursors(view, block_iter_get_offset(&view->cursor), len, 0);
    }
}

void buffer_delete_bytes(View *view, size_t len)
{
    buffer_delete_bytes_internal(view, len, false);
}

void buffer_erase_bytes(View *view, size_t len)
{
    buffer_delete_bytes_internal(view, len, true);
}

void buffer_replace_bytes(View *view, size_t del_count, const char *ins, size_t ins_count)
{
    view_reset_preferred_x(view);
    if (del_count == 0) {
        buffer_insert_bytes(view, ins, ins_count);
        return;
    }
    if (ins_count == 0) {
        buffer_delete_bytes(view, del_count);
        return;
    }

    // Check if all newlines from EOF would be deleted
    if (would_delete_last_bytes(view, del_count)) {
        if (ins[ins_count - 1] != '\n') {
            // Don't replace last newline
            if (--del_count == 0) {
                buffer_insert_bytes(view, ins, ins_count);
                return;
            }
        }
    }

    char *deleted = do_replace(view, del_count, ins, ins_count);
    record_replace(view, deleted, del_count, ins_count);

    if (view->buffer->views.count > 1) {
        fix_cursors(view, block_iter_get_offset(&view->cursor), del_count, ins_count);
    }
}
