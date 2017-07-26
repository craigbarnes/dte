#include "file-location.h"
#include "window.h"
#include "view.h"
#include "search.h"
#include "move.h"

static PTR_ARRAY(file_locations);

struct file_location *create_file_location(struct view *v)
{
	struct file_location *loc;

	loc = xnew0(struct file_location, 1);
	loc->filename = v->buffer->abs_filename ? xstrdup(v->buffer->abs_filename) : NULL;
	loc->buffer_id = v->buffer->id;
	loc->line = v->cy + 1;
	loc->column = v->cx_char + 1;
	return loc;
}

void file_location_free(struct file_location *loc)
{
	free(loc->filename);
	free(loc->pattern);
	free(loc);
}

bool file_location_equals(const struct file_location *a, const struct file_location *b)
{
	if (!xstreq(a->filename, b->filename)) {
		return false;
	}
	if (a->buffer_id != b->buffer_id) {
		return false;
	}
	if (!xstreq(a->pattern, b->pattern)) {
		return false;
	}
	if (a->line != b->line) {
		return false;
	}
	if (a->column != b->column) {
		return false;
	}
	return true;
}

bool file_location_go(struct file_location *loc)
{
	struct window *w = window;
	struct view *v = window_open_buffer(w, loc->filename, true, NULL);
	bool ok = true;

	if (!v) {
		// failed to open file. error message should be visible
		return false;
	}
	if (w->view != v) {
		set_view(v);
		// force centering view to the cursor because file changed
		v->force_center = true;
	}
	if (loc->pattern != NULL) {
		bool err = false;
		search_tag(loc->pattern, &err);
		ok = !err;
	} else if (loc->line > 0) {
		move_to_line(v, loc->line);
		if (loc->column > 0) {
			move_to_column(v, loc->column);
		}
	}
	return ok;
}

bool file_location_return(struct file_location *loc)
{
	struct window *w = window;
	struct buffer *b = find_buffer_by_id(loc->buffer_id);
	struct view *v;

	if (b != NULL) {
		v = window_get_view(w, b);
	} else {
		if (loc->filename == NULL) {
			// Can't restore closed buffer which had no filename.
			// Try again.
			return false;
		}
		v = window_open_buffer(w, loc->filename, true, NULL);
	}
	if (v == NULL) {
		// Open failed. Don't try again.
		return true;
	}
	set_view(v);
	move_to_line(v, loc->line);
	move_to_column(v, loc->column);
	return true;
}

void push_file_location(struct file_location *loc)
{
	ptr_array_add(&file_locations, loc);
}

void pop_file_location(void)
{
	bool go = true;

	while (file_locations.count > 0 && go) {
		struct file_location *loc = file_locations.ptrs[--file_locations.count];
		go = !file_location_return(loc);
		file_location_free(loc);
	}
}
