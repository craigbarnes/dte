#ifndef BLOCK_ITER_H
#define BLOCK_ITER_H

#include <stdbool.h>
#include <stddef.h>
#include "block.h"
#include "util/list.h"
#include "util/macros.h"
#include "util/string-view.h"
#include "util/unicode.h"

// An iterator used to represent the cursor position for each View of a
// Buffer (see View::cursor) and also as a means for accessing/iterating
// the Blocks that make up the text contents of a Buffer (see Buffer::blocks).
typedef struct {
    Block *blk; // The Block this iterator/cursor currently points to
    const ListHead *head; // A pointer to the Buffer::blocks that owns `blk`
    size_t offset; // The current position within `blk->data`
} BlockIter;

typedef struct {
    StringView line;
    size_t cursor_offset;
} CurrentLineRef;

static inline void block_iter_bof(BlockIter *bi)
{
    bi->blk = BLOCK(bi->head->next);
    bi->offset = 0;
}

static inline void block_iter_eof(BlockIter *bi)
{
    bi->blk = BLOCK(bi->head->prev);
    bi->offset = bi->blk->size;
}

static inline bool block_iter_is_eof(const BlockIter *bi)
{
    return bi->offset == bi->blk->size && bi->blk->node.next == bi->head;
}

static inline bool block_iter_is_bol(const BlockIter *bi)
{
    // See also: Block invariants mentioned in sanity_check_blocks()
    return bi->offset == 0 || bi->blk->data[bi->offset - 1] == '\n';
}

static inline bool block_iter_is_eol(const BlockIter *bi)
{
    const Block *blk = bi->blk;
    size_t offset = bi->offset;
    if (offset == blk->size) {
        if (blk->node.next == bi->head) {
            // EOF
            return true;
        }

        // Normalize
        blk = BLOCK(blk->node.next);
        offset = 0;
    }

    return blk->data[offset] == '\n';
}

static inline void block_iter_normalize(BlockIter *bi)
{
    const Block *blk = bi->blk;
    if (bi->offset == blk->size && blk->node.next != bi->head) {
        bi->blk = BLOCK(blk->node.next);
        bi->offset = 0;
    }
}

size_t block_iter_eat_line(BlockIter *bi) NONNULL_ARGS;
size_t block_iter_next_line(BlockIter *bi) NONNULL_ARGS;
size_t block_iter_prev_line(BlockIter *bi) NONNULL_ARGS;
size_t block_iter_prev_line_eol(BlockIter *bi) NONNULL_ARGS;
size_t block_iter_next_char(BlockIter *bi, CodePoint *up) NONNULL_ARGS READWRITE(1) WRITEONLY(2);
size_t block_iter_prev_char(BlockIter *bi, CodePoint *up) NONNULL_ARGS READWRITE(1) WRITEONLY(2);
size_t block_iter_next_column(BlockIter *bi) NONNULL_ARGS;
size_t block_iter_prev_column(BlockIter *bi) NONNULL_ARGS;
size_t block_iter_bol(BlockIter *bi) NONNULL_ARGS;
size_t block_iter_eol(BlockIter *bi) NONNULL_ARGS;
size_t block_iter_skip_blanks_fwd(BlockIter *bi) NONNULL_ARGS;
size_t block_iter_skip_blanks_bwd(BlockIter *bi) NONNULL_ARGS;
bool block_iter_find_non_empty_line_bwd(BlockIter *bi) NONNULL_ARGS;
void block_iter_back_bytes(BlockIter *bi, size_t count) NONNULL_ARGS;
void block_iter_skip_bytes(BlockIter *bi, size_t count) NONNULL_ARGS;
void block_iter_goto_offset(BlockIter *bi, size_t offset) NONNULL_ARGS;
void block_iter_goto_line(BlockIter *bi, size_t line) NONNULL_ARGS;
size_t block_iter_get_offset(const BlockIter *bi) WARN_UNUSED_RESULT NONNULL_ARGS;
size_t block_iter_get_char(const BlockIter *bi, CodePoint *up) WARN_UNUSED_RESULT NONNULL_ARGS READONLY(1) WRITEONLY(2);
char *block_iter_get_bytes(const BlockIter *bi, size_t len) WARN_UNUSED_RESULT NONNULL_ARGS;
StringView block_iter_get_line_with_nl(BlockIter *bi) NONNULL_ARGS;

// Like block_iter_get_line_with_nl(), but excluding the newline
static inline StringView block_iter_get_line(BlockIter *bi)
{
    StringView line = block_iter_get_line_with_nl(bi);
    line.length -= (line.length > 0); // Trim the newline (if any)
    return line;
}

// Like block_iter_get_line(), but always returning whole lines
static inline StringView get_current_line(BlockIter tmp)
{
    block_iter_bol(&tmp);
    return block_iter_get_line(&tmp);
}

// Like get_current_line(), but returning a struct that also includes the
// cursor offset (relative to BOL)
static inline CurrentLineRef get_current_line_and_offset(BlockIter tmp)
{
    size_t offset = block_iter_bol(&tmp);
    return (CurrentLineRef) {
        .line = block_iter_get_line(&tmp),
        .cursor_offset = offset,
    };
}

#endif
