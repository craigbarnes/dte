#ifndef BUFFER_H
#define BUFFER_H

#include "iter.h"
#include "list.h"
#include "options.h"
#include "common.h"
#include "ptr-array.h"

struct change {
	struct change *next;
	struct change **prev;
	unsigned int nr_prev;

	// move after inserted text when undoing delete?
	bool move_after;

	long offset;
	long del_count;
	long ins_count;

	// deleted bytes (inserted bytes need not to be saved)
	char *buf;
};

struct buffer {
	struct list_head blocks;
	struct change change_head;
	struct change *cur_change;

	// used to determine if buffer is modified
	struct change *saved_change;

	struct stat st;

	// needed for identifying buffers whose filename is NULL
	unsigned int id;

	long nl;

	// views pointing to this buffer
	struct ptr_array views;

	char *display_filename;
	char *abs_filename;

	bool ro;
	bool locked;
	bool setup;

	enum newline_sequence newline;

	// Encoding of the file. Buffer always contains UTF-8.
	char *encoding;

	struct local_options options;

	struct syntax *syn;
	// Index 0 is always syn->states.ptrs[0].
	// Lowest bit of an invalidated value is 1.
	struct ptr_array line_start_states;

	int changed_line_min;
	int changed_line_max;
};

// buffer = view->buffer = window->view->buffer
extern struct view *view;
extern struct buffer *buffer;
extern struct ptr_array buffers;
extern bool everything_changed;

static inline void mark_all_lines_changed(struct buffer *b)
{
	b->changed_line_min = 0;
	b->changed_line_max = INT_MAX;
}

static inline void mark_everything_changed(void)
{
	everything_changed = true;
}

static inline bool buffer_modified(struct buffer *b)
{
	return b->saved_change != b->cur_change;
}

void buffer_mark_lines_changed(struct buffer *b, int min, int max);
const char *buffer_filename(struct buffer *b);

void update_short_filename_cwd(struct buffer *b, const char *cwd);
void update_short_filename(struct buffer *b);
struct buffer *find_buffer(const char *abs_filename);
struct buffer *find_buffer_by_id(unsigned int id);
struct buffer *buffer_new(const char *encoding);
struct buffer *open_empty_buffer(void);
void free_buffer(struct buffer *b);
bool buffer_detect_filetype(struct buffer *b);
void buffer_update_syntax(struct buffer *b);
void buffer_setup(struct buffer *b);

long buffer_get_char(struct block_iter *bi, unsigned int *up);
long buffer_next_char(struct block_iter *bi, unsigned int *up);
long buffer_prev_char(struct block_iter *bi, unsigned int *up);

#endif
