#include "change.h"
#include "buffer.h"
#include "error.h"
#include "block.h"
#include "view.h"

static enum change_merge change_merge;
static enum change_merge prev_change_merge;

static Change *alloc_change(void)
{
    return xcalloc(sizeof(Change));
}

static void add_change(Change *change)
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

static Change *new_change(void)
{
    Change *change;

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
        add_change(change_barrier);
        change_barrier = NULL;
    }

    change = alloc_change();
    add_change(change);
    return change;
}

static size_t buffer_offset(void)
{
    return block_iter_get_offset(&view->cursor);
}

static void record_insert(size_t len)
{
    Change *change = buffer->cur_change;

    BUG_ON(!len);
    if (
        change_merge == prev_change_merge
        && change_merge == CHANGE_MERGE_INSERT
    ) {
        BUG_ON(change->del_count);
        change->ins_count += len;
        return;
    }

    change = new_change();
    change->offset = buffer_offset();
    change->ins_count = len;
}

static void record_delete(char *buf, size_t len, bool move_after)
{
    Change *change = buffer->cur_change;

    BUG_ON(!len);
    BUG_ON(!buf);
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

    change = new_change();
    change->offset = buffer_offset();
    change->del_count = len;
    change->move_after = move_after;
    change->buf = buf;
}

static void record_replace(char *deleted, size_t del_count, size_t ins_count)
{
    BUG_ON(del_count && !deleted);
    BUG_ON(!del_count && deleted);
    BUG_ON(!del_count && !ins_count);

    Change *change = new_change();
    change->offset = buffer_offset();
    change->ins_count = ins_count;
    change->del_count = del_count;
    change->buf = deleted;
}

void begin_change(enum change_merge m)
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

void end_change_chain(void)
{
    if (change_barrier) {
        // There were no changes in this change chain.
        free(change_barrier);
        change_barrier = NULL;
    } else {
        // There were some changes. Add end of chain marker.
        add_change(alloc_change());
    }
}

static void fix_cursors(size_t offset, size_t del, size_t ins)
{
    for (size_t i = 0; i < buffer->views.count; i++) {
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

static void reverse_change(Change *change)
{
    if (buffer->views.count > 1) {
        fix_cursors(change->offset, change->ins_count, change->del_count);
    }

    block_iter_goto_offset(&view->cursor, change->offset);
    if (!change->ins_count) {
        // Convert delete to insert
        do_insert(change->buf, change->del_count);
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
        char *buf = do_replace(del_count, change->buf, ins_count);

        free(change->buf);
        change->buf = buf;
        change->ins_count = ins_count;
        change->del_count = del_count;
    } else {
        // Convert insert to delete
        change->buf = do_delete(change->ins_count);
        change->del_count = change->ins_count;
        change->ins_count = 0;
    }
}

bool undo(void)
{
    Change *change = buffer->cur_change;

    view_reset_preferred_x(view);
    if (!change->next) {
        return false;
    }

    if (is_change_chain_barrier(change)) {
        int count = 0;

        while (1) {
            change = change->next;
            if (is_change_chain_barrier(change)) {
                break;
            }
            reverse_change(change);
            count++;
        }
        if (count > 1) {
            info_msg("Undid %d changes.", count);
        }
    } else {
        reverse_change(change);
    }
    buffer->cur_change = change->next;
    return true;
}

bool redo(unsigned int change_id)
{
    Change *change = buffer->cur_change;

    view_reset_preferred_x(view);
    if (!change->prev) {
        // Don't complain if change_id is 0
        if (change_id) {
            error_msg("Nothing to redo.");
        }
        return false;
    }

    if (change_id) {
        if (--change_id >= change->nr_prev) {
            error_msg (
                "There are only %u possible changes to redo.",
                change->nr_prev
            );
            return false;
        }
    } else {
        // Default to newest change
        change_id = change->nr_prev - 1;
        if (change->nr_prev > 1) {
            info_msg (
                "Redoing newest (%u) of %u possible changes.",
                change_id + 1u,
                change->nr_prev
            );
        }
    }

    change = change->prev[change_id];
    if (is_change_chain_barrier(change)) {
        int count = 0;

        while (1) {
            change = change->prev[change->nr_prev - 1];
            if (is_change_chain_barrier(change)) {
                break;
            }
            reverse_change(change);
            count++;
        }
        if (count > 1) {
            info_msg("Redid %d changes.", count);
        }
    } else {
        reverse_change(change);
    }
    buffer->cur_change = change;
    return true;
}

void free_changes(Change *ch)
{
top:
    while (ch->nr_prev) {
        ch = ch->prev[ch->nr_prev - 1];
    }

    // ch is leaf now
    while (ch->next) {
        Change *next = ch->next;

        free(ch->buf);
        free(ch);

        ch = next;
        if (--ch->nr_prev) {
            goto top;
        }

        // We have become leaf
        free(ch->prev);
    }
}

void buffer_insert_bytes(const char *buf, const size_t len)
{
    size_t rec_len = len;

    view_reset_preferred_x(view);
    if (len == 0) {
        return;
    }

    if (buf[len - 1] != '\n' && block_iter_is_eof(&view->cursor)) {
        // Force newline at EOF
        do_insert("\n", 1);
        rec_len++;
    }

    do_insert(buf, len);
    record_insert(rec_len);

    if (buffer->views.count > 1) {
        fix_cursors(block_iter_get_offset(&view->cursor), len, 0);
    }
}

static bool would_delete_last_bytes(size_t count)
{
    Block *blk = view->cursor.blk;
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

static void buffer_delete_bytes_internal(size_t len, bool move_after)
{
    view_reset_preferred_x(view);
    if (len == 0) {
        return;
    }

    // Check if all newlines from EOF would be deleted
    if (would_delete_last_bytes(len)) {
        BlockIter bi = view->cursor;
        unsigned int u;
        if (buffer_prev_char(&bi, &u) && u != '\n') {
            // No newline before cursor
            if (--len == 0) {
                begin_change(CHANGE_MERGE_NONE);
                return;
            }
        }
    }
    record_delete(do_delete(len), len, move_after);

    if (buffer->views.count > 1) {
        fix_cursors(block_iter_get_offset(&view->cursor), len, 0);
    }
}

void buffer_delete_bytes(size_t len)
{
    buffer_delete_bytes_internal(len, false);
}

void buffer_erase_bytes(size_t len)
{
    buffer_delete_bytes_internal(len, true);
}

void buffer_replace_bytes (
    size_t del_count,
    const char *const inserted,
    size_t ins_count
) {
    view_reset_preferred_x(view);
    if (del_count == 0) {
        buffer_insert_bytes(inserted, ins_count);
        return;
    }
    if (ins_count == 0) {
        buffer_delete_bytes(del_count);
        return;
    }

    // Check if all newlines from EOF would be deleted
    if (would_delete_last_bytes(del_count)) {
        if (inserted[ins_count - 1] != '\n') {
            // Don't replace last newline
            if (--del_count == 0) {
                buffer_insert_bytes(inserted, ins_count);
                return;
            }
        }
    }

    char *deleted = do_replace(del_count, inserted, ins_count);
    record_replace(deleted, del_count, ins_count);

    if (buffer->views.count > 1) {
        fix_cursors(block_iter_get_offset(&view->cursor), del_count, ins_count);
    }
}
