#include "edit.h"
#include "block.h"
#include "buffer.h"
#include "syntax/highlight.h"
#include "util/debug.h"
#include "util/list.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "view.h"

#define BLOCK_EDIT_SIZE 512

static void sanity_check_blocks(const View *v, bool check_newlines)
{
#if DEBUG >= 1
    const Buffer *b = v->buffer;
    BUG_ON(list_empty(&b->blocks));
    BUG_ON(v->cursor.offset > v->cursor.blk->size);

    const Block *blk = BLOCK(b->blocks.next);
    if (blk->size == 0) {
        // The only time a zero-sized block is valid is when it's the
        // first and only block
        BUG_ON(b->blocks.next->next != &b->blocks);
        BUG_ON(v->cursor.blk != blk);
        return;
    }

    bool cursor_seen = false;
    block_for_each(blk, &b->blocks) {
        const size_t size = blk->size;
        BUG_ON(size == 0);
        BUG_ON(size > blk->alloc);
        if (blk == v->cursor.blk) {
            cursor_seen = true;
        }
        if (check_newlines) {
            BUG_ON(blk->data[size - 1] != '\n');
        }
        if (DEBUG > 2) {
            BUG_ON(count_nl(blk->data, size) != blk->nl);
        }
    }
    BUG_ON(!cursor_seen);
#else
    // Silence "unused parameter" warnings
    (void)v;
    (void)check_newlines;
#endif
}

static size_t copy_count_nl(char *dst, const char *src, size_t len)
{
    size_t nl = 0;
    for (size_t i = 0; i < len; i++) {
        dst[i] = src[i];
        if (src[i] == '\n') {
            nl++;
        }
    }
    return nl;
}

static size_t insert_to_current(View *v, const char *buf, size_t len)
{
    Block *blk = v->cursor.blk;
    size_t offset = v->cursor.offset;
    size_t size = blk->size + len;

    if (size > blk->alloc) {
        blk->alloc = round_size_to_next_multiple(size, BLOCK_ALLOC_MULTIPLE);
        xrenew(blk->data, blk->alloc);
    }
    memmove(blk->data + offset + len, blk->data + offset, blk->size - offset);
    size_t nl = copy_count_nl(blk->data + offset, buf, len);
    blk->nl += nl;
    blk->size = size;
    return nl;
}

/*
 * Combine current block and new data into smaller blocks:
 *   - Block _must_ contain whole lines
 *   - Block _must_ contain at least one line
 *   - Preferred maximum size of block is BLOCK_EDIT_SIZE
 *   - Size of any block can be larger than BLOCK_EDIT_SIZE
 *     only if there's a very long line
 */
static size_t split_and_insert(View *v, const char *buf, size_t len)
{
    Block *blk = v->cursor.blk;
    ListHead *prev_node = blk->node.prev;
    const char *buf1 = blk->data;
    const char *buf2 = buf;
    const char *buf3 = blk->data + v->cursor.offset;
    size_t size1 = v->cursor.offset;
    size_t size2 = len;
    size_t size3 = blk->size - size1;
    size_t total = size1 + size2 + size3;
    size_t start = 0; // Beginning of new block
    size_t size = 0; // Size of new block
    size_t pos = 0; // Current position
    size_t nl_added = 0;

    while (start < total) {
        // Size of new block if next line would be added
        size_t new_size = 0;
        size_t copied = 0;

        if (pos < size1) {
            const char *nl = memchr(buf1 + pos, '\n', size1 - pos);
            if (nl) {
                new_size = nl - buf1 + 1 - start;
            }
        }

        if (!new_size && pos < size1 + size2) {
            size_t offset = 0;
            if (pos > size1) {
                offset = pos - size1;
            }

            const char *nl = memchr(buf2 + offset, '\n', size2 - offset);
            if (nl) {
                new_size = size1 + nl - buf2 + 1 - start;
            }
        }

        if (!new_size && pos < total) {
            size_t offset = 0;
            if (pos > size1 + size2) {
                offset = pos - size1 - size2;
            }

            const char *nl = memchr(buf3 + offset, '\n', size3 - offset);
            if (nl) {
                new_size = size1 + size2 + nl - buf3 + 1 - start;
            } else {
                new_size = total - start;
            }
        }

        if (new_size <= BLOCK_EDIT_SIZE) {
            // Fits
            size = new_size;
            pos = start + new_size;
            if (pos < total) {
                continue;
            }
        } else {
            // Does not fit
            if (!size) {
                // One block containing one very long line
                size = new_size;
                pos = start + new_size;
            }
        }

        BUG_ON(!size);
        Block *new = block_new(size);
        if (start < size1) {
            size_t avail = size1 - start;
            size_t count = MIN(size, avail);
            new->nl += copy_count_nl(new->data, buf1 + start, count);
            copied += count;
            start += count;
        }
        if (start >= size1 && start < size1 + size2) {
            size_t offset = start - size1;
            size_t avail = size2 - offset;
            size_t count = MIN(size - copied, avail);
            new->nl += copy_count_nl(new->data + copied, buf2 + offset, count);
            copied += count;
            start += count;
        }
        if (start >= size1 + size2) {
            size_t offset = start - size1 - size2;
            size_t avail = size3 - offset;
            size_t count = size - copied;
            BUG_ON(count > avail);
            new->nl += copy_count_nl(new->data + copied, buf3 + offset, count);
            copied += count;
            start += count;
        }

        new->size = size;
        BUG_ON(copied != size);
        list_add_before(&new->node, &blk->node);

        nl_added += new->nl;
        size = 0;
    }

    v->cursor.blk = BLOCK(prev_node->next);
    while (v->cursor.offset > v->cursor.blk->size) {
        v->cursor.offset -= v->cursor.blk->size;
        v->cursor.blk = BLOCK(v->cursor.blk->node.next);
    }

    nl_added -= blk->nl;
    block_free(blk);
    return nl_added;
}

static size_t insert_bytes(View *v, const char *buf, size_t len)
{
    // Blocks must contain whole lines.
    // Last char of buf might not be newline.
    block_iter_normalize(&v->cursor);

    Block *blk = v->cursor.blk;
    size_t new_size = blk->size + len;
    if (new_size <= blk->alloc || new_size <= BLOCK_EDIT_SIZE) {
        return insert_to_current(v, buf, len);
    }

    if (blk->nl <= 1 && !memchr(buf, '\n', len)) {
        // Can't split this possibly very long line.
        // insert_to_current() is much faster than split_and_insert().
        return insert_to_current(v, buf, len);
    }
    return split_and_insert(v, buf, len);
}

void do_insert(View *v, const char *buf, size_t len)
{
    Buffer *b = v->buffer;
    size_t nl = insert_bytes(v, buf, len);
    b->nl += nl;
    sanity_check_blocks(v, true);

    view_update_cursor_y(v);
    buffer_mark_lines_changed(b, v->cy, nl ? LONG_MAX : v->cy);
    if (b->syn) {
        hl_insert(b, v->cy, nl);
    }
}

static bool only_block(const Buffer *b, const Block *blk)
{
    return blk->node.prev == &b->blocks && blk->node.next == &b->blocks;
}

char *do_delete(View *v, size_t len, bool sanity_check_newlines)
{
    ListHead *saved_prev_node = NULL;
    Block *blk = v->cursor.blk;
    size_t offset = v->cursor.offset;
    size_t pos = 0;
    size_t deleted_nl = 0;

    if (!len) {
        return NULL;
    }

    if (!offset) {
        // The block where cursor is can become empty and thereby may be deleted
        saved_prev_node = blk->node.prev;
    }

    Buffer *b = v->buffer;
    char *deleted = xmalloc(len);
    while (pos < len) {
        ListHead *next = blk->node.next;
        size_t avail = blk->size - offset;
        size_t count = MIN(len - pos, avail);
        size_t nl = copy_count_nl(deleted + pos, blk->data + offset, count);
        if (count < avail) {
            memmove (
                blk->data + offset,
                blk->data + offset + count,
                avail - count
            );
        }

        deleted_nl += nl;
        b->nl -= nl;
        blk->nl -= nl;
        blk->size -= count;
        if (!blk->size && !only_block(b, blk)) {
            block_free(blk);
        }

        offset = 0;
        pos += count;
        blk = BLOCK(next);

        BUG_ON(pos < len && next == &b->blocks);
    }

    if (saved_prev_node) {
        // Cursor was at beginning of a block that was possibly deleted
        if (saved_prev_node->next == &b->blocks) {
            v->cursor.blk = BLOCK(saved_prev_node);
            v->cursor.offset = v->cursor.blk->size;
        } else {
            v->cursor.blk = BLOCK(saved_prev_node->next);
        }
    }

    blk = v->cursor.blk;
    if (
        blk->size
        && blk->data[blk->size - 1] != '\n'
        && blk->node.next != &b->blocks
    ) {
        Block *next = BLOCK(blk->node.next);
        size_t size = blk->size + next->size;

        if (size > blk->alloc) {
            blk->alloc = round_size_to_next_multiple(size, BLOCK_ALLOC_MULTIPLE);
            xrenew(blk->data, blk->alloc);
        }
        memcpy(blk->data + blk->size, next->data, next->size);
        blk->size = size;
        blk->nl += next->nl;
        block_free(next);
    }

    sanity_check_blocks(v, sanity_check_newlines);

    view_update_cursor_y(v);
    buffer_mark_lines_changed(b, v->cy, deleted_nl ? LONG_MAX : v->cy);
    if (b->syn) {
        hl_delete(b, v->cy, deleted_nl);
    }
    return deleted;
}

char *do_replace(View *v, size_t del, const char *buf, size_t ins)
{
    block_iter_normalize(&v->cursor);
    Block *blk = v->cursor.blk;
    size_t offset = v->cursor.offset;

    size_t avail = blk->size - offset;
    if (del >= avail) {
        goto slow;
    }

    size_t new_size = blk->size + ins - del;
    if (new_size > BLOCK_EDIT_SIZE) {
        // Should split
        if (blk->nl > 1 || memchr(buf, '\n', ins)) {
            // Most likely can be split
            goto slow;
        }
    }

    if (new_size > blk->alloc) {
        blk->alloc = round_size_to_next_multiple(new_size, BLOCK_ALLOC_MULTIPLE);
        xrenew(blk->data, blk->alloc);
    }

    // Modification is limited to one block
    Buffer *b = v->buffer;
    char *ptr = blk->data + offset;
    char *deleted = xmalloc(del);
    size_t del_nl = copy_count_nl(deleted, ptr, del);
    blk->nl -= del_nl;
    b->nl -= del_nl;

    if (del != ins) {
        memmove(ptr + ins, ptr + del, avail - del);
    }

    size_t ins_nl = copy_count_nl(ptr, buf, ins);
    blk->nl += ins_nl;
    b->nl += ins_nl;
    blk->size = new_size;
    sanity_check_blocks(v, true);
    view_update_cursor_y(v);

    // If the number of inserted and removed bytes are the same, some
    // line(s) changed but the lines after them didn't move up or down
    long max = (del_nl == ins_nl) ? v->cy + del_nl : LONG_MAX;
    buffer_mark_lines_changed(b, v->cy, max);

    if (b->syn) {
        hl_delete(b, v->cy, del_nl);
        hl_insert(b, v->cy, ins_nl);
    }

    return deleted;

slow:
    // The "sanity_check_newlines" argument of do_delete() is false here
    // because it may be removing a terminating newline that do_insert()
    // is going to insert again at a different position:
    deleted = do_delete(v, del, false);
    do_insert(v, buf, ins);
    return deleted;
}
