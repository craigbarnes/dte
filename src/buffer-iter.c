#include "buffer.h"
#include "uchar.h"

long buffer_get_char(struct block_iter *bi, unsigned int *up)
{
	struct block_iter tmp = *bi;
	return buffer_next_char(&tmp, up);
}

long buffer_next_char(struct block_iter *bi, unsigned int *up)
{
	long offset = bi->offset;

	if (offset == bi->blk->size) {
		if (bi->blk->node.next == bi->head)
			return 0;
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

long buffer_prev_char(struct block_iter *bi, unsigned int *up)
{
	long offset = bi->offset;

	if (!offset) {
		if (bi->blk->node.prev == bi->head)
			return 0;
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
