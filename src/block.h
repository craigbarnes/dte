#ifndef BLOCK_H
#define BLOCK_H

#include <stddef.h>
#include "util/debug.h"
#include "util/list.h"
#include "util/macros.h"

// Blocks always contain whole lines.
// There's one zero-sized block for an empty file.
// Otherwise zero-sized blocks are forbidden.
typedef struct {
    ListHead node;
    char NONSTRING *data;
    size_t size;
    size_t alloc;
    size_t nl;
} Block;

enum {
    BLOCK_ALLOC_MULTIPLE = 64,
};

#define block_for_each(block_, list_head_) \
    for ( \
        block_ = BLOCK((list_head_)->next); \
        &block_->node != (list_head_); \
        block_ = BLOCK(block_->node.next) \
    )

// NOLINTNEXTLINE(readability-identifier-naming)
static inline Block *BLOCK(ListHead *item)
{
    static_assert(offsetof(Block, node) == 0);
    return (Block*)item;
}

static inline void block_sanity_check(const Block *blk)
{
    BUG_ON(!blk);
    BUG_ON(blk->size > blk->alloc);
    BUG_ON(blk->nl > blk->size);

    // block_new() forbids `alloc == 0` and thus always allocates
    // at least BLOCK_ALLOC_MULTIPLE bytes
    BUG_ON(blk->alloc < BLOCK_ALLOC_MULTIPLE);
    BUG_ON(!blk->data);
}

Block *block_new(size_t alloc) RETURNS_NONNULL;
void block_grow(Block *blk, size_t alloc) NONNULL_ARGS;
void block_free(Block *blk) NONNULL_ARGS;

#endif
