#ifndef ITER_H
#define ITER_H

#include "libc.h"
#include "list.h"

/*
 * struct block always contains whole lines.
 *
 * There's one zero-sized block when the file is empty. Otherwise
 * zero-sized blocks are forbidden.
 */
struct block {
	struct list_head node;
	unsigned char *data;
	long size;
	long alloc;
	long nl;
};

static inline struct block *BLOCK(struct list_head *item)
{
	return container_of(item, struct block, node);
}

struct block_iter {
	struct block *blk;
	struct list_head *head;
	long offset;
};

struct lineref {
	const unsigned char *line;
	long size;
};

#define BLOCK_ITER(name, head) struct block_iter name = { BLOCK((head)->next), (head), 0 }

static inline void block_iter_bof(struct block_iter *bi)
{
	bi->blk = BLOCK(bi->head->next);
	bi->offset = 0;
}

static inline void block_iter_eof(struct block_iter *bi)
{
	bi->blk = BLOCK(bi->head->prev);
	bi->offset = bi->blk->size;
}

void block_iter_normalize(struct block_iter *bi);
long block_iter_eat_line(struct block_iter *bi);
long block_iter_next_line(struct block_iter *bi);
long block_iter_prev_line(struct block_iter *bi);
long block_iter_bol(struct block_iter *bi);
long block_iter_eol(struct block_iter *bi);
void block_iter_back_bytes(struct block_iter *bi, long count);
void block_iter_skip_bytes(struct block_iter *bi, long count);
void block_iter_goto_offset(struct block_iter *bi, long offset);
void block_iter_goto_line(struct block_iter *bi, long line);
long block_iter_get_offset(const struct block_iter *bi);
bool block_iter_is_bol(const struct block_iter *bi);
char *block_iter_get_bytes(const struct block_iter *bi, long len);

static inline bool block_iter_is_eof(struct block_iter *bi)
{
	return bi->offset == bi->blk->size && bi->blk->node.next == bi->head;
}

void fill_line_ref(struct block_iter *bi, struct lineref *lr);
void fill_line_nl_ref(struct block_iter *bi, struct lineref *lr);
long fetch_this_line(const struct block_iter *bi, struct lineref *lr);

#endif
