#ifndef BLOCK_H
#define BLOCK_H

#include <stddef.h>
#include "util/list.h"
#include "util/macros.h"

// Blocks always contain whole lines.
// There's one zero-sized block when the file is empty.
// Otherwise zero-sized blocks are forbidden.
typedef struct {
    ListHead node;
    unsigned char NONSTRING *data;
    size_t size;
    size_t alloc;
    size_t nl;
} Block;

static inline Block *BLOCK(const ListHead *item)
{
    static_assert(offsetof(Block, node) == 0);
    return (Block*)item;
}

#define block_for_each(block_, list_head_) \
    for ( \
        block_ = BLOCK((list_head_)->next); \
        &block_->node != (list_head_); \
        block_ = BLOCK(block_->node.next) \
    )

Block *block_new(size_t size);
void do_insert(const char *buf, size_t len);
char *do_delete(size_t len);
char *do_replace(size_t del, const char *buf, size_t ins);

#endif
