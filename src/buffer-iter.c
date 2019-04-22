#include "buffer.h"
#include "util/utf8.h"

size_t buffer_get_char(BlockIter *bi, CodePoint *up)
{
    BlockIter tmp = *bi;
    return buffer_next_char(&tmp, up);
}

size_t buffer_next_char(BlockIter *bi, CodePoint *up)
{
    size_t offset = bi->offset;

    if (offset == bi->blk->size) {
        if (bi->blk->node.next == bi->head) {
            return 0;
        }
        bi->blk = BLOCK(bi->blk->node.next);
        bi->offset = offset = 0;
    }

    // Note: this block can't be empty
    *up = bi->blk->data[offset];
    if (*up < 0x80) {
        bi->offset++;
        return 1;
    }

    *up = u_get_nonascii(bi->blk->data, bi->blk->size, &bi->offset);
    return bi->offset - offset;
}

size_t buffer_prev_char(BlockIter *bi, CodePoint *up)
{
    size_t offset = bi->offset;

    if (!offset) {
        if (bi->blk->node.prev == bi->head) {
            return 0;
        }
        bi->blk = BLOCK(bi->blk->node.prev);
        bi->offset = offset = bi->blk->size;
    }

    // Note: this block can't be empty
    *up = bi->blk->data[offset - 1];
    if (*up < 0x80) {
        bi->offset--;
        return 1;
    }

    *up = u_prev_char(bi->blk->data, &bi->offset);
    return offset - bi->offset;
}

size_t buffer_next_column(BlockIter *bi)
{
    CodePoint u;
    size_t size = buffer_next_char(bi, &u);
    while (buffer_get_char(bi, &u) && u_is_nonspacing_mark(u)) {
        size += buffer_next_char(bi, &u);
    }
    return size;
}

size_t buffer_prev_column(BlockIter *bi)
{
    CodePoint u;
    size_t skip, total = 0;
    do {
        skip = buffer_prev_char(bi, &u);
        total += skip;
    } while (skip && u_is_nonspacing_mark(u));
    return total;
}
