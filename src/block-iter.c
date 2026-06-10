#include <string.h>
#include "block-iter.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/utf8.h"
#include "util/xmalloc.h"
#include "util/xmemrchr.h"

// Move to end of previous line (if any) and return number of bytes moved
static size_t block_iter_prev_line_eol(BlockIter *bi)
{
    BlockIter tmp = *bi;
    size_t n = block_iter_bol(&tmp);
    if (block_iter_is_bof(&tmp)) {
        return 0; // Already on first line; leave `bi` unchanged
    }

    CodePoint u;
    *bi = tmp;
    return n + block_iter_prev_char(bi, &u);
}

// Move to beginning of previous line (if any) and return number of bytes moved
size_t block_iter_prev_line(BlockIter *bi)
{
    size_t n = block_iter_prev_line_eol(bi);
    return n ? n + block_iter_bol(bi) : 0;
}

size_t block_iter_get_char(const BlockIter *bi, CodePoint *up)
{
    BlockIter tmp = *bi;
    return block_iter_next_char(&tmp, up);
}

size_t block_iter_next_char(BlockIter *bi, CodePoint *up)
{
    if (unlikely(bi->offset == bi->blk->size && !block_iter_next_block(bi))) {
        return 0; // Already at EOF
    }

    BUG_ON(bi->blk->size == 0); // This block can't be empty

    unsigned char byte = bi->blk->data[bi->offset];
    if (likely(byte < 0x80)) {
        *up = byte;
        bi->offset++;
        return 1;
    }

    size_t prev_offset = bi->offset;
    *up = u_get_nonascii(bi->blk->data, bi->blk->size, &bi->offset);
    return bi->offset - prev_offset;
}

size_t block_iter_prev_char(BlockIter *bi, CodePoint *up)
{
    if (unlikely(bi->offset == 0 && !block_iter_end_of_prev_block(bi))) {
        return 0; // Already at BOF
    }

    BUG_ON(bi->blk->size == 0); // This block can't be empty
    BUG_ON(bi->offset == 0);

    unsigned char byte = bi->blk->data[bi->offset - 1];
    if (likely(byte < 0x80)) {
        *up = byte;
        bi->offset--;
        return 1;
    }

    size_t prev_offset = bi->offset;
    *up = u_prev_char(bi->blk->data, &bi->offset);
    return prev_offset - bi->offset;
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
    if (block_iter_is_bol(bi)) {
        return 0;
    }

    // These cases are handled by the condition above
    const Block *blk = bi->blk;
    size_t offset = bi->offset;
    BUG_ON(offset == 0);
    BUG_ON(offset >= blk->size);

    if (blk->nl == 1) {
        bi->offset = 0; // Only 1 line in Block; bol is at offset 0
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
    size_t offset = bi->offset;
    if (unlikely(offset == blk->size)) {
        return 0; // Already at EOF
    }

    BUG_ON(blk->size == 0); // This block can't be empty
    BUG_ON(blk->nl == 0);

    if (blk->nl == 1) {
        bi->offset = blk->size - 1;
        return bi->offset - offset;
    }

    StringView line = buf_slice_next_line(blk->data, &offset, blk->size);
    bi->offset += line.length;
    return line.length;
}

// Move after next newline (beginning of next line or end of file) and
// return number of bytes moved
size_t block_iter_eat_line(BlockIter *bi)
{
    CodePoint u;
    size_t n = block_iter_eol(bi);
    size_t m = block_iter_next_char(bi, &u);
    BUG_ON(m && (m != 1 || u != '\n'));
    return n + m;
}

// Move to beginning of next line (if any) and return number of bytes moved
size_t block_iter_next_line(BlockIter *bi)
{
    BlockIter tmp = *bi;
    size_t move = block_iter_eat_line(&tmp);
    if (unlikely(block_iter_is_eof(&tmp))) {
        return 0;
    }

    *bi = tmp;
    return move;
}

// Count spaces and tabs at or after iterator (and move beyond them)
size_t block_iter_skip_blanks_fwd(BlockIter *bi)
{
    block_iter_normalize(bi);
    StringView sv = strview_from_slice(bi->blk->data, bi->offset, bi->blk->size);

    // We're only operating on one line and checking for ASCII characters,
    // so Block traversal and Unicode-aware decoding are both unnecessary
    size_t count = strview_blank_prefix_length(sv);

    bi->offset += count + 1;
    return count;
}

// Count spaces and tabs before iterator (and move to beginning of them)
size_t block_iter_skip_blanks_bwd(BlockIter *bi)
{
    block_iter_normalize(bi);
    StringView sv = string_view(bi->blk->data, bi->offset);
    size_t count = strview_blank_suffix_length(sv);
    bi->offset -= count;
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
        bool have_prev_block = block_iter_end_of_prev_block(bi);
        BUG_ON(!have_prev_block);
    }
    bi->offset -= count;
}

void block_iter_skip_bytes(BlockIter *bi, size_t count)
{
    size_t avail = bi->blk->size - bi->offset;
    while (count > avail) {
        count -= avail;
        bool have_next_block = block_iter_next_block(bi);
        BUG_ON(!have_next_block);
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
    Block *blk = block_iter_get_first_block(bi);
    size_t nl = 0;
    while (blk->node.next != bi->head && nl + blk->nl < line) {
        nl += blk->nl;
        blk = block_next(blk);
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

char *block_iter_get_bytes(BlockIter bi, size_t len)
{
    if (len == 0) {
        return NULL;
    }

    size_t pos = 0;
    char *buf = xmalloc(len + 1); // +1 byte; so expand_word() can append '\0'

    while (pos < len) {
        const size_t avail = bi.blk->size - bi.offset;
        size_t count = MIN(len - pos, avail);
        memcpy(buf + pos, bi.blk->data + bi.offset, count);
        pos += count;
        bool have_next_block = block_iter_next_block(&bi);
        BUG_ON(pos < len && !have_next_block);
    }

    return buf;
}

// Return the contents of the line that extends from `bi`. Callers
// should ensure `bi` is already at BOL, if whole lines are needed.
StringView block_iter_get_line_with_nl(BlockIter *bi)
{
    block_iter_normalize(bi);
    if (unlikely(bi->offset == bi->blk->size)) {
        // Cursor at end of last block
        return strview("");
    }

    StringView line;
    if (bi->blk->nl == 1) {
        // Block contains only 1 line; end-of-line is end-of-block
        line = strview_from_slice(bi->blk->data, bi->offset, bi->blk->size);
    } else {
        size_t pos = bi->offset;
        line = buf_slice_next_line(bi->blk->data, &pos, bi->blk->size);
        line.length += 1; // Include the newline
    }

    BUG_ON(!strview_has_suffix(line, "\n"));
    return line;
}
