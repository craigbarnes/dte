#include "buffer.h"
#include "view.h"
#include "editor.h"
#include "change.h"
#include "block.h"
#include "filetype.h"
#include "state.h"
#include "syntax.h"
#include "file-option.h"
#include "lock.h"
#include "selection.h"
#include "path.h"
#include "unicode.h"
#include "uchar.h"
#include "detect.h"

struct buffer *buffer;
PTR_ARRAY(buffers);
bool everything_changed;

static void set_display_filename(struct buffer *b, char *name)
{
	free(b->display_filename);
	b->display_filename = name;
}

/*
 * Mark line range min...max (inclusive) "changed". These lines will be
 * redrawn when screen is updated. This is called even when content has not
 * been changed, but selection has or line has been deleted and all lines
 * after the deleted line move up.
 *
 * Syntax highlighter has different logic. It cares about contents of the
 * lines, not about selection or if the lines have been moved up or down.
 */
void buffer_mark_lines_changed(struct buffer *b, int min, int max)
{
	if (min > max) {
		int tmp = min;
		min = max;
		max = tmp;
	}
	if (min < b->changed_line_min)
		b->changed_line_min = min;
	if (max > b->changed_line_max)
		b->changed_line_max = max;
}

const char *buffer_filename(struct buffer *b)
{
	return b->display_filename;
}

struct buffer *buffer_new(const char *encoding)
{
	static int id;
	struct buffer *b;

	b = xnew0(struct buffer, 1);
	list_init(&b->blocks);
	b->cur_change = &b->change_head;
	b->saved_change = &b->change_head;
	b->id = ++id;
	b->newline = options.newline;
	if (encoding)
		b->encoding = xstrdup(encoding);

	memcpy(&b->options, &options, sizeof(struct common_options));
	b->options.brace_indent = 0;
	b->options.filetype = xstrdup("none");
	b->options.indent_regex = xstrdup("");

	ptr_array_add(&buffers, b);
	return b;
}

struct buffer *open_empty_buffer(void)
{
	struct buffer *b = buffer_new(charset);
	struct block *blk;

	// at least one block required
	blk = block_new(1);
	list_add_before(&blk->node, &b->blocks);

	set_display_filename(b, xstrdup("(No name)"));
	return b;
}

void free_buffer(struct buffer *b)
{
	struct list_head *item;

	ptr_array_remove(&buffers, b);

	if (b->locked)
		unlock_file(b->abs_filename);

	item = b->blocks.next;
	while (item != &b->blocks) {
		struct list_head *next = item->next;
		struct block *blk = BLOCK(item);

		free(blk->data);
		free(blk);
		item = next;
	}
	free_changes(&b->change_head);
	free(b->line_start_states.ptrs);
	free(b->views.ptrs);
	free(b->display_filename);
	free(b->abs_filename);
	free(b->encoding);
	free_local_options(&b->options);
	free(b);
}

static int same_file(const struct stat *a, const struct stat *b)
{
	return a->st_dev == b->st_dev && a->st_ino == b->st_ino;
}

struct buffer *find_buffer(const char *abs_filename)
{
	struct stat st;
	bool st_ok = stat(abs_filename, &st) == 0;
	int i;

	for (i = 0; i < buffers.count; i++) {
		struct buffer *b = buffers.ptrs[i];
		const char *f = b->abs_filename;

		if ((f != NULL && streq(f, abs_filename)) || (st_ok && same_file(&st, &b->st))) {
			return b;
		}
	}
	return NULL;
}

struct buffer *find_buffer_by_id(unsigned int id)
{
	int i;

	for (i = 0; i < buffers.count; i++) {
		struct buffer *b = buffers.ptrs[i];
		if (b->id == id) {
			return b;
		}
	}
	return NULL;
}

bool buffer_detect_filetype(struct buffer *b)
{
	char *interpreter = detect_interpreter(b);
	const char *ft = NULL;

	if (BLOCK(b->blocks.next)->size) {
		BLOCK_ITER(bi, &b->blocks);
		struct lineref lr;

		fill_line_ref(&bi, &lr);
		ft = find_ft(b->abs_filename, interpreter, lr.line, lr.size);
	} else if (b->abs_filename) {
		ft = find_ft(b->abs_filename, interpreter, NULL, 0);
	}
	free(interpreter);

	if (ft && !streq(ft, b->options.filetype)) {
		free(b->options.filetype);
		b->options.filetype = xstrdup(ft);
		return true;
	}
	return false;
}

void update_short_filename_cwd(struct buffer *b, const char *cwd)
{
	if (b->abs_filename) {
		if (cwd) {
			set_display_filename(b, short_filename_cwd(b->abs_filename, cwd));
		} else {
			// getcwd() failed
			set_display_filename(b, xstrdup(b->abs_filename));
		}
	}
}

void update_short_filename(struct buffer *b)
{
	set_display_filename(b, short_filename(b->abs_filename));
}

void buffer_update_syntax(struct buffer *b)
{
	struct syntax *syn = NULL;

	if (b->options.syntax) {
		/* even "none" can have syntax */
		syn = find_syntax(b->options.filetype);
		if (!syn)
			syn = load_syntax_by_filetype(b->options.filetype);
	}
	if (syn == b->syn)
		return;

	b->syn = syn;
	if (syn) {
		// start state of first line is constant
		struct ptr_array *s = &b->line_start_states;
		if (!s->alloc) {
			s->alloc = 64;
			s->ptrs = xnew(void *, s->alloc);
		}
		s->ptrs[0] = syn->states.ptrs[0];
		s->count = 1;
	}

	mark_all_lines_changed(b);
}

void buffer_setup(struct buffer *b)
{
	b->setup = true;
	buffer_detect_filetype(b);
	set_file_options(b);
	buffer_update_syntax(b);
	if (b->options.detect_indent && b->abs_filename != NULL) {
		detect_indent(b);
	}
}
