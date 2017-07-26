#include "change.h"
#include "buffer.h"
#include "error.h"
#include "block.h"
#include "view.h"

static enum change_merge change_merge;
static enum change_merge prev_change_merge;

static struct change *alloc_change(void)
{
	return xcalloc(sizeof(struct change));
}

static void add_change(struct change *change)
{
	struct change *head = buffer->cur_change;

	change->next = head;
	xrenew(head->prev, head->nr_prev + 1);
	head->prev[head->nr_prev++] = change;

	buffer->cur_change = change;
}

/* This doesn't need to be local to buffer because commands are atomic. */
static struct change *change_barrier;

static bool is_change_chain_barrier(struct change *change)
{
	return !change->ins_count && !change->del_count;
}

static struct change *new_change(void)
{
	struct change *change;

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

static long buffer_offset(void)
{
	return block_iter_get_offset(&view->cursor);
}

static void record_insert(long len)
{
	struct change *change = buffer->cur_change;

	BUG_ON(!len);
	if (change_merge == prev_change_merge && change_merge == CHANGE_MERGE_INSERT) {
		BUG_ON(change->del_count);
		change->ins_count += len;
		return;
	}

	change = new_change();
	change->offset = buffer_offset();
	change->ins_count = len;
}

static void record_delete(char *buf, long len, bool move_after)
{
	struct change *change = buffer->cur_change;

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

static void record_replace(char *deleted, long del_count, long ins_count)
{
	struct change *change;

	BUG_ON(del_count && !deleted);
	BUG_ON(!del_count && deleted);
	BUG_ON(!del_count && !ins_count);

	change = new_change();
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

	/*
	 * Allocate change chain barrier but add it to the change tree only if
	 * there will be any real changes
	 */
	change_barrier = alloc_change();
	change_merge = CHANGE_MERGE_NONE;
}

void end_change_chain(void)
{
	if (change_barrier) {
		/* There were no changes in this change chain. */
		free(change_barrier);
		change_barrier = NULL;
	} else {
		/* There were some changes. Add end of chain marker. */
		add_change(alloc_change());
	}
}

static void fix_cursors(long offset, long del, long ins)
{
	int i;

	for (i = 0; i < buffer->views.count; i++) {
		struct view *v = buffer->views.ptrs[i];

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

static void reverse_change(struct change *change)
{
	if (buffer->views.count > 1)
		fix_cursors(change->offset, change->ins_count, change->del_count);

	block_iter_goto_offset(&view->cursor, change->offset);
	if (!change->ins_count) {
		// convert delete to insert
		do_insert(change->buf, change->del_count);
		if (change->move_after)
			block_iter_skip_bytes(&view->cursor, change->del_count);
		change->ins_count = change->del_count;
		change->del_count = 0;
		free(change->buf);
		change->buf = NULL;
	} else if (change->del_count) {
		// reverse replace
		long del_count = change->ins_count;
		long ins_count = change->del_count;
		char *buf = do_replace(del_count, change->buf, ins_count);

		free(change->buf);
		change->buf = buf;
		change->ins_count = ins_count;
		change->del_count = del_count;
	} else {
		// convert insert to delete
		change->buf = do_delete(change->ins_count);
		change->del_count = change->ins_count;
		change->ins_count = 0;
	}
}

bool undo(void)
{
	struct change *change = buffer->cur_change;

	view_reset_preferred_x(view);
	if (!change->next)
		return false;

	if (is_change_chain_barrier(change)) {
		int count = 0;

		while (1) {
			change = change->next;
			if (is_change_chain_barrier(change))
				break;
			reverse_change(change);
			count++;
		}
		if (count > 1)
			info_msg("Undid %d changes.", count);
	} else {
		reverse_change(change);
	}
	buffer->cur_change = change->next;
	return true;
}

bool redo(unsigned int change_id)
{
	struct change *change = buffer->cur_change;

	view_reset_preferred_x(view);
	if (!change->prev) {
		/* don't complain if change_id is 0 */
		if (change_id)
			error_msg("Nothing to redo.");
		return false;
	}

	if (change_id) {
		if (--change_id >= change->nr_prev) {
			error_msg("There are only %d possible changes to redo.", change->nr_prev);
			return false;
		}
	} else {
		/* default to newest change  */
		change_id = change->nr_prev - 1;
		if (change->nr_prev > 1)
			info_msg("Redoing newest (%d) of %d possible changes.", change_id + 1, change->nr_prev);
	}

	change = change->prev[change_id];
	if (is_change_chain_barrier(change)) {
		int count = 0;

		while (1) {
			change = change->prev[change->nr_prev - 1];
			if (is_change_chain_barrier(change))
				break;
			reverse_change(change);
			count++;
		}
		if (count > 1)
			info_msg("Redid %d changes.", count);
	} else {
		reverse_change(change);
	}
	buffer->cur_change = change;
	return true;
}

void free_changes(struct change *ch)
{
top:
	while (ch->nr_prev)
		ch = ch->prev[ch->nr_prev - 1];

	// ch is leaf now
	while (ch->next) {
		struct change *next = ch->next;

		free(ch->buf);
		free(ch);

		ch = next;
		if (--ch->nr_prev)
			goto top;

		// we have become leaf
		free(ch->prev);
	}
}

void buffer_insert_bytes(const char *buf, long len)
{
	long rec_len = len;

	view_reset_preferred_x(view);
	if (len == 0)
		return;

	if (buf[len - 1] != '\n' && block_iter_is_eof(&view->cursor)) {
		// force newline at EOF
		do_insert("\n", 1);
		rec_len++;
	}

	do_insert(buf, len);
	record_insert(rec_len);

	if (buffer->views.count > 1)
		fix_cursors(block_iter_get_offset(&view->cursor), len, 0);
}

static bool would_delete_last_bytes(long count)
{
	struct block *blk = view->cursor.blk;
	long offset = view->cursor.offset;

	while (1) {
		long avail = blk->size - offset;

		if (avail > count)
			return false;

		if (blk->node.next == view->cursor.head)
			return true;

		count -= avail;
		blk = BLOCK(blk->node.next);
		offset = 0;
	}
}

static void buffer_delete_bytes_internal(long len, bool move_after)
{
	view_reset_preferred_x(view);
	if (len == 0)
		return;

	// check if all newlines from EOF would be deleted
	if (would_delete_last_bytes(len)) {
		struct block_iter bi = view->cursor;
		unsigned int u;

		if (buffer_prev_char(&bi, &u) && u != '\n') {
			// no newline before cursor
			if (--len == 0) {
				begin_change(CHANGE_MERGE_NONE);
				return;
			}
		}
	}
	record_delete(do_delete(len), len, move_after);

	if (buffer->views.count > 1)
		fix_cursors(block_iter_get_offset(&view->cursor), len, 0);
}

void buffer_delete_bytes(long len)
{
	buffer_delete_bytes_internal(len, false);
}

void buffer_erase_bytes(long len)
{
	buffer_delete_bytes_internal(len, true);
}

void buffer_replace_bytes(long del_count, const char *inserted, long ins_count)
{
	char *deleted = NULL;

	view_reset_preferred_x(view);
	if (del_count == 0) {
		buffer_insert_bytes(inserted, ins_count);
		return;
	}
	if (ins_count == 0) {
		buffer_delete_bytes(del_count);
		return;
	}

	// check if all newlines from EOF would be deleted
	if (would_delete_last_bytes(del_count)) {
		if (inserted[ins_count - 1] != '\n') {
			// don't replace last newline
			if (--del_count == 0) {
				buffer_insert_bytes(inserted, ins_count);
				return;
			}
		}
	}

	deleted = do_replace(del_count, inserted, ins_count);
	record_replace(deleted, del_count, ins_count);

	if (buffer->views.count > 1)
		fix_cursors(block_iter_get_offset(&view->cursor), del_count, ins_count);
}
