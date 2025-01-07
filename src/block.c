#include <stdlib.h>
#include "block.h"
#include "util/xmalloc.h"

enum {
    BLOCK_ALLOC_MULTIPLE = 64
};

Block *block_new(size_t alloc)
{
    Block *blk = xnew0(Block, 1);
    alloc = next_multiple(alloc, BLOCK_ALLOC_MULTIPLE);
    blk->data = xmalloc(alloc);
    blk->alloc = alloc;
    return blk;
}

void block_grow(Block *blk, size_t alloc)
{
    if (alloc > blk->alloc) {
        blk->alloc = next_multiple(alloc, BLOCK_ALLOC_MULTIPLE);
        blk->data = xrealloc(blk->data, blk->alloc);
    }
}

void block_free(Block *blk)
{
    list_remove(&blk->node);
    free(blk->data);
    free(blk);
}
