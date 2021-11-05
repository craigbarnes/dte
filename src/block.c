#include <stdlib.h>
#include "block.h"
#include "util/xmalloc.h"

Block *block_new(size_t alloc)
{
    Block *blk = xnew0(Block, 1);
    alloc = round_size_to_next_multiple(alloc, BLOCK_ALLOC_MULTIPLE);
    blk->data = xmalloc(alloc);
    blk->alloc = alloc;
    return blk;
}

void block_free(Block *blk)
{
    list_del(&blk->node);
    free(blk->data);
    free(blk);
}
