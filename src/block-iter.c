#include <string.h>
#include "block-iter.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/utf8.h"
#include "util/xmalloc.h"
#include "util/xmemrchr.h"

NONNULL_ARGS_AND_RETURN
static const char *block_memchr_eol(const Block *blk, size_t offset)
{
    BUG_ON(offset > blk->size);
    const char *eol = memchr(blk->data + offset, '\n', blk->size - offset);
    BUG_ON(!eol);
    return eol;
}

// Move after next newline (beginning of next line or end of file)
// and return number of bytes moved
size_t block_iter_eat_line(BlockIter *bi)
{
    block_iter_normalize(bi);
    const size_t offset = bi->offset;
    if (unlikely(offset == bi->blk->size)) {
        return 0;
    }

    // There must be at least one newline
    if (bi->blk->nl == 1) {
        bi->offset = bi->blk->size;
    } else {
        const char *end = block_memchr_eol(bi->blk, offset);
        bi->offset = (size_t)(end + 1 - bi->blk->data);
    }

    return bi->offset - offset;
}

// Move to beginning of next line (if any) and return number of bytes moved
size_t block_iter_next_line(BlockIter *bi)
{
    block_iter_normalize(bi);
    const size_t offset = bi->offset;
    if (unlikely(offset == bi->blk->size)) {
        return 0;
    }

    // There must be at least one newline
    size_t new_offset;
    if (bi->blk->nl == 1) {
        new_offset = bi->blk->size;
    } else {
        const char *end = block_memchr_eol(bi->blk, offset);
        new_offset = (size_t)(end + 1 - bi->blk->data);
    }

    if (new_offset == bi->blk->size && bi->blk->node.next == bi->head) {
        return 0;
    }

    bi->offset = new_offset;
    return bi->offset - offset;
}

// Move to beginning of previous line (if any) and return number of bytes moved
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

size_t block_iter_get_char(const BlockIter *bi, CodePoint *up)
{
    BlockIter tmp = *bi;
    return block_iter_next_char(&tmp, up);
}

size_t block_iter_next_char(BlockIter *bi, CodePoint *up)
{
    size_t offset = bi->offset;
    if (unlikely(offset == bi->blk->size)) {
        if (unlikely(bi->blk->node.next == bi->head)) {
            return 0;
        }
        bi->blk = BLOCK(bi->blk->node.next);
        bi->offset = offset = 0;
    }

    // Note: this block can't be empty
    unsigned char byte = bi->blk->data[offset];
    if (likely(byte < 0x80)) {
        *up = byte;
        bi->offset++;
        return 1;
    }

    *up = u_get_nonascii(bi->blk->data, bi->blk->size, &bi->offset);
    return bi->offset - offset;
}

size_t block_iter_prev_char(BlockIter *bi, CodePoint *up)
{
    size_t offset = bi->offset;
    if (unlikely(offset == 0)) {
        if (unlikely(bi->blk->node.prev == bi->head)) {
            return 0;
        }
        bi->blk = BLOCK(bi->blk->node.prev);
        bi->offset = offset = bi->blk->size;
    }

    // Note: this block can't be empty
    unsigned char byte = bi->blk->data[offset - 1];
    if (likely(byte < 0x80)) {
        *up = byte;
        bi->offset--;
        return 1;
    }

    *up = u_prev_char(bi->blk->data, &bi->offset);
    return offset - bi->offset;
}

size_t block_iter_next_column(BlockIter *bi)
{
    CodePoint u;
    size_t size = block_iter_next_char(bi, &u);
    while (block_iter_get_char(bi, &u) && u_is_zero_width(u)) {
        size += block_iter_next_char(bi, &u);
    }
    return size;
}

size_t block_iter_prev_column(BlockIter *bi)
{
    CodePoint u;
    size_t skip, total = 0;
    do {
        skip = block_iter_prev_char(bi, &u);
        total += skip;
    } while (skip && u_is_zero_width(u));
    return total;
}

size_t block_iter_bol(BlockIter *bi)
{
    block_iter_normalize(bi);
    size_t offset = bi->offset;
    if (block_iter_is_bol(bi)) {
        return 0;
    }

    // These cases are handled by the condition above
    const Block *blk = bi->blk;
    BUG_ON(offset == 0);
    BUG_ON(offset >= blk->size);

    if (blk->nl == 1) {
        bi->offset = 0; // Only 1 line in `blk`; bol is at offset 0
        return offset;
    }

    const char *nl = xmemrchr(blk->data, '\n', offset - 1);
    if (!nl) {
        bi->offset = 0; // No newline before offset; bol is at offset 0
        return offset;
    }

    offset = (size_t)(nl - blk->data) + 1;
    size_t count = bi->offset - offset;
    bi->offset = offset;
    return count;
}

size_t block_iter_eol(BlockIter *bi)
{
    block_iter_normalize(bi);
    const Block *blk = bi->blk;
    const size_t offset = bi->offset;

    if (unlikely(offset == blk->size)) {
        // Cursor at end of last block
        return 0;
    }

    if (blk->nl == 1) {
        bi->offset = blk->size - 1;
        return bi->offset - offset;
    }

    const char *end = block_memchr_eol(blk, offset);
    bi->offset = (size_t)(end - blk->data);
    return bi->offset - offset;
}

// Count spaces and tabs at or after iterator (and move beyond them)
size_t block_iter_skip_blanks_fwd(BlockIter *bi)
{
    block_iter_normalize(bi);
    const char *data = bi->blk->data;
    size_t count = 0;
    size_t i = bi->offset;

    // We're only operating on one line and checking for ASCII characters,
    // so Block traversal and Unicode-aware decoding are both unnecessary
    for (size_t n = bi->blk->size; i < n; count++) {
        unsigned char c = data[i++];
        if (!ascii_isblank(c)) {
            break;
        }
    }

    bi->offset = i;
    return count;
}

// Count spaces and tabs before iterator (and move to beginning of them)
size_t block_iter_skip_blanks_bwd(BlockIter *bi)

{
    block_iter_normalize(bi);
    size_t count = 0;
    size_t i = bi->offset;

    for (const char *data = bi->blk->data; i > 0; count++) {
        unsigned char c = data[--i];
        if (!ascii_isblank(c)) {
            i++;
            break;
        }
    }

    bi->offset = i;
    return count;
}

// Non-empty line can be used to determine size of indentation for the next line
bool block_iter_find_non_empty_line_bwd(BlockIter *bi)
{
    block_iter_bol(bi);
    do {
        StringView line = block_iter_get_line(bi);
        if (!strview_isblank(line)) {
            return true;
        }
    } while (block_iter_prev_line(bi));
    return false;
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
    block_for_each(blk, bi->head) {
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
    const Block *blk;
    size_t offset = 0;
    block_for_each(blk, bi->head) {
        if (blk == bi->blk) {
            break;
        }
        offset += blk->size;
    }
    return offset + bi->offset;
}

char *block_iter_get_bytes(const BlockIter *bi, size_t len)
{
    if (len == 0) {
        return NULL;
    }

    const Block *blk = bi->blk;
    size_t offset = bi->offset;
    size_t pos = 0;
    char *buf = xmalloc(len + 1); // +1 byte; so expand_word() can append '\0'

    while (pos < len) {
        const size_t avail = blk->size - offset;
        size_t count = MIN(len - pos, avail);
        memcpy(buf + pos, blk->data + offset, count);
        pos += count;
        BUG_ON(pos < len && blk->node.next == bi->head);
        blk = BLOCK(blk->node.next);
        offset = 0;
    }

    return buf;
}

// Return the contents of the line that extends from `bi`. Callers
// should ensure `bi` is already at BOL, if whole lines are needed.
StringView block_iter_get_line_with_nl(BlockIter *bi)
{
    block_iter_normalize(bi);
    const char *start = bi->blk->data + bi->offset;
    StringView line = {.data = start, .length = 0};
    size_t max = bi->blk->size - bi->offset;

    if (unlikely(max == 0)) {
        // Cursor at end of last block
        return line;
    }

    if (bi->blk->nl == 1) {
        // Block contains only 1 line; end-of-line is end-of-block
        BUG_ON(line.data[max - 1] != '\n');
        line.length = max;
        return line;
    }

    const char *nl = block_memchr_eol(bi->blk, bi->offset);
    line.length = (size_t)(nl - start + 1);
    BUG_ON(line.length == 0);
    return line;
}
