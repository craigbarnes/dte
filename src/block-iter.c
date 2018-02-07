#include "block-iter.h"
#include "common.h"

void block_iter_normalize(BlockIter *bi)
{
    Block *blk = bi->blk;

    if (bi->offset == blk->size && blk->node.next != bi->head) {
        bi->blk = BLOCK(blk->node.next);
        bi->offset = 0;
    }
}

/*
 * Move after next newline (beginning of next line or end of file).
 * Returns number of bytes iterator advanced.
 */
size_t block_iter_eat_line(BlockIter *bi)
{
    size_t offset;

    block_iter_normalize(bi);

    offset = bi->offset;
    if (offset == bi->blk->size) {
        return 0;
    }

    // There must be at least one newline
    if (bi->blk->nl == 1) {
        bi->offset = bi->blk->size;
    } else {
        const unsigned char *end;
        end = memchr(bi->blk->data + offset, '\n', bi->blk->size - offset);
        bi->offset = end + 1 - bi->blk->data;
    }
    return bi->offset - offset;
}

/*
 * Move to beginning of next line.
 * If there is no next line iterator is not advanced.
 * Returns number of bytes iterator advanced.
 */
size_t block_iter_next_line(BlockIter *bi)
{
    size_t offset;
    size_t new_offset;

    block_iter_normalize(bi);

    offset = bi->offset;
    if (offset == bi->blk->size) {
        return 0;
    }

    // There must be at least one newline
    if (bi->blk->nl == 1) {
        new_offset = bi->blk->size;
    } else {
        const unsigned char *end;
        end = memchr(bi->blk->data + offset, '\n', bi->blk->size - offset);
        new_offset = end + 1 - bi->blk->data;
    }
    if (new_offset == bi->blk->size && bi->blk->node.next == bi->head) {
        return 0;
    }

    bi->offset = new_offset;
    return bi->offset - offset;
}

/*
 * Move to beginning of previous line.
 * Returns number of bytes moved which is zero if there's no previous line.
 */
size_t block_iter_prev_line(BlockIter *bi)
{
    Block *blk = bi->blk;
    size_t offset = bi->offset;
    size_t start = offset;

    while (offset && blk->data[offset - 1] != '\n') {
        offset--;
    }

    if (!offset) {
        if (blk->node.prev == bi->head) {
            return 0;
        }
        bi->blk = blk = BLOCK(blk->node.prev);
        offset = blk->size;
        start += offset;
    }

    offset--;
    while (offset && blk->data[offset - 1] != '\n') {
        offset--;
    }
    bi->offset = offset;
    return start - offset;
}

size_t block_iter_bol(BlockIter *bi)
{
    size_t offset, ret;

    block_iter_normalize(bi);

    offset = bi->offset;
    if (!offset || offset == bi->blk->size) {
        return 0;
    }

    if (bi->blk->nl == 1) {
        offset = 0;
    } else {
        while (offset && bi->blk->data[offset - 1] != '\n') {
            offset--;
        }
    }

    ret = bi->offset - offset;
    bi->offset = offset;
    return ret;
}

size_t block_iter_eol(BlockIter *bi)
{
    Block *blk;
    size_t offset;
    const unsigned char *end;

    block_iter_normalize(bi);

    blk = bi->blk;
    offset = bi->offset;
    if (offset == blk->size) {
        // Cursor at end of last block
        return 0;
    }
    if (blk->nl == 1) {
        bi->offset = blk->size - 1;
        return bi->offset - offset;
    }
    end = memchr(blk->data + offset, '\n', blk->size - offset);
    bi->offset = end - blk->data;
    return bi->offset - offset;
}

void block_iter_back_bytes(BlockIter *bi, size_t count)
{
    while (count > bi->offset) {
        count -= bi->offset;
        bi->blk = BLOCK(bi->blk->node.prev);
        bi->offset = bi->blk->size;
    }
    bi->offset -= count;
}

void block_iter_skip_bytes(BlockIter *bi, size_t count)
{
    size_t avail = bi->blk->size - bi->offset;

    while (count > avail) {
        count -= avail;
        bi->blk = BLOCK(bi->blk->node.next);
        bi->offset = 0;
        avail = bi->blk->size;
    }
    bi->offset += count;
}

void block_iter_goto_offset(BlockIter *bi, size_t offset)
{
    Block *blk;

    list_for_each_entry(blk, bi->head, node) {
        if (offset <= blk->size) {
            bi->blk = blk;
            bi->offset = offset;
            return;
        }
        offset -= blk->size;
    }
}

void block_iter_goto_line(BlockIter *bi, size_t line)
{
    Block *blk = BLOCK(bi->head->next);
    size_t nl = 0;

    while (blk->node.next != bi->head && nl + blk->nl < line) {
        nl += blk->nl;
        blk = BLOCK(blk->node.next);
    }

    bi->blk = blk;
    bi->offset = 0;
    while (nl < line) {
        if (!block_iter_eat_line(bi)) {
            break;
        }
        nl++;
    }
}

size_t block_iter_get_offset(const BlockIter *bi)
{
    Block *blk;
    size_t offset = 0;

    list_for_each_entry(blk, bi->head, node) {
        if (blk == bi->blk) {
            break;
        }
        offset += blk->size;
    }
    return offset + bi->offset;
}

bool block_iter_is_bol(const BlockIter *bi)
{
    size_t offset = bi->offset;

    if (!offset) {
        return true;
    }
    return bi->blk->data[offset - 1] == '\n';
}

char *block_iter_get_bytes(const BlockIter *bi, size_t len)
{
    Block *blk = bi->blk;
    size_t offset = bi->offset;
    size_t pos = 0;
    char *buf;

    if (!len) {
        return NULL;
    }

    buf = xnew(char, len);
    while (pos < len) {
        size_t avail = blk->size - offset;
        size_t count = len - pos;

        if (count > avail) {
            count = avail;
        }
        memcpy(buf + pos, blk->data + offset, count);
        pos += count;

        BUG_ON(pos < len && blk->node.next == bi->head);
        blk = BLOCK(blk->node.next);
        offset = 0;
    }
    return buf;
}

// bi should be at bol
void fill_line_ref(BlockIter *bi, LineRef *lr)
{
    size_t max;
    const unsigned char *nl;

    block_iter_normalize(bi);

    lr->line = bi->blk->data + bi->offset;
    max = bi->blk->size - bi->offset;
    if (max == 0) {
        // Cursor at end of last block
        lr->size = 0;
        return;
    }
    if (bi->blk->nl == 1) {
        lr->size = max - 1;
        return;
    }
    nl = memchr(lr->line, '\n', max);
    lr->size = nl - lr->line;
}

void fill_line_nl_ref(BlockIter *bi, LineRef *lr)
{
    size_t max;
    const unsigned char *nl;

    block_iter_normalize(bi);

    lr->line = bi->blk->data + bi->offset;
    max = bi->blk->size - bi->offset;
    if (max == 0) {
        // Cursor at end of last block
        lr->size = 0;
        return;
    }
    if (bi->blk->nl == 1) {
        lr->size = max;
        return;
    }
    nl = memchr(lr->line, '\n', max);
    lr->size = nl - lr->line + 1;
}

size_t fetch_this_line(const BlockIter *bi, LineRef *lr)
{
    BlockIter tmp = *bi;
    size_t count = block_iter_bol(&tmp);

    fill_line_ref(&tmp, lr);
    return count;
}
