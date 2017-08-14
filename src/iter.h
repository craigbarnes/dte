#ifndef ITER_H
#define ITER_H

#include "libc.h"
#include "list.h"

/*
 * Block always contains whole lines.
 *
 * There's one zero-sized block when the file is empty. Otherwise
 * zero-sized blocks are forbidden.
 */
typedef struct block {
    ListHead node;
    unsigned char *data;
    long size;
    long alloc;
    long nl;
} Block;

static inline Block *BLOCK(ListHead *item)
{
    return container_of(item, Block, node);
}

typedef struct {
    Block *blk;
    ListHead *head;
    long offset;
} BlockIter;

typedef struct {
    const unsigned char *line;
    long size;
} LineRef;

#define BLOCK_ITER_INIT(head) {BLOCK((head)->next), (head), 0}

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

void block_iter_normalize(BlockIter *bi);
long block_iter_eat_line(BlockIter *bi);
long block_iter_next_line(BlockIter *bi);
long block_iter_prev_line(BlockIter *bi);
long block_iter_bol(BlockIter *bi);
long block_iter_eol(BlockIter *bi);
void block_iter_back_bytes(BlockIter *bi, long count);
void block_iter_skip_bytes(BlockIter *bi, long count);
void block_iter_goto_offset(BlockIter *bi, long offset);
void block_iter_goto_line(BlockIter *bi, long line);
long block_iter_get_offset(const BlockIter *bi);
bool block_iter_is_bol(const BlockIter *bi);
char *block_iter_get_bytes(const BlockIter *bi, long len);

static inline bool block_iter_is_eof(BlockIter *bi)
{
    return bi->offset == bi->blk->size && bi->blk->node.next == bi->head;
}

void fill_line_ref(BlockIter *bi, LineRef *lr);
void fill_line_nl_ref(BlockIter *bi, LineRef *lr);
long fetch_this_line(const BlockIter *bi, LineRef *lr);

#endif
