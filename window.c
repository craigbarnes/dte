#include "window.h"
#include "file-history.h"

PTR_ARRAY(windows);
struct window *window;

struct window *window_new(void)
{
	struct window *w = xnew0(struct window, 1);

	ptr_array_add(&windows, w);
	return w;
}

struct view *window_add_buffer(struct buffer *b)
{
	struct view *v = xnew0(struct view, 1);

	ptr_array_add(&b->views, v);
	v->buffer = b;
	v->window = window;
	ptr_array_add(&window->views, v);
	mark_tabbar_changed();
	return v;
}

void view_delete(struct view *v)
{
	struct buffer *b = v->buffer;

	if (v == prev_view)
		prev_view = NULL;

	ptr_array_remove(&b->views, ptr_array_idx(&b->views, v));
	if (b->views.count == 0) {
		if (b->options.file_history && b->abs_filename)
			add_file_history(v->cy + 1, v->cx_char + 1, b->abs_filename);
		free_buffer(b);
	}
	free(v);
	mark_tabbar_changed();
}

void remove_view(void)
{
	int idx = view_idx();

	ptr_array_remove(&window->views, idx);
	view_delete(view);
	view = NULL;

	if (prev_view) {
		set_view(prev_view);
		return;
	}
	if (window->views.count == 0)
		open_empty_buffer();
	if (window->views.count == idx)
		idx--;
	set_view(window->views.ptrs[idx]);
}

void set_view(struct view *v)
{
	if (view == v)
		return;

	/* forget previous view when changing view using any other command but open */
	prev_view = NULL;

	view = v;
	buffer = v->buffer;
	window = v->window;

	window->view = v;

	if (!buffer->setup)
		setup_buffer();
}

static int cursor_outside_view(void)
{
	return view->cy < view->vy || view->cy > view->vy + window->edit_h - 1;
}

static void center_view_to_cursor(void)
{
	unsigned int hh = window->edit_h / 2;

	if (window->edit_h >= buffer->nl || view->cy < hh) {
		view->vy = 0;
		return;
	}

	view->vy = view->cy - hh;
	if (view->vy + window->edit_h > buffer->nl) {
		/* -1 makes one ~ line visible so that you know where the EOF is */
		view->vy -= view->vy + window->edit_h - buffer->nl - 1;
	}
}

static void update_view_x(void)
{
	unsigned int c = 8;

	if (view->cx_display - view->vx >= window->edit_w)
		view->vx = (view->cx_display - window->edit_w + c) / c * c;
	if (view->cx_display < view->vx)
		view->vx = view->cx_display / c * c;
}

static void update_view_y(void)
{
	int margin = get_scroll_margin();
	int max_y = view->vy + window->edit_h - 1 - margin;

	if (view->cy < view->vy + margin) {
		view->vy = view->cy - margin;
		if (view->vy < 0)
			view->vy = 0;
	} else if (view->cy > max_y) {
		view->vy += view->cy - max_y;
		max_y = buffer->nl - window->edit_h + 1;
		if (view->vy > max_y && max_y >= 0)
			view->vy = max_y;
	}
}

void update_view(void)
{
	update_view_x();
	if (view->force_center || (view->center_on_scroll && cursor_outside_view()))
		center_view_to_cursor();
	else
		update_view_y();

	view->force_center = 0;
	view->center_on_scroll = 0;
}

void calculate_line_numbers(struct window *win)
{
	int w = 0, min_w = 5;

	if (options.show_line_numbers && win->view) {
		w = number_width(win->view->buffer->nl) + 1;
		if (w < min_w)
			w = min_w;
	}
	if (w != win->line_numbers.width) {
		win->line_numbers.width = w;
		win->line_numbers.first = 0;
		win->line_numbers.last = 0;
		mark_all_lines_changed();
	}

	win->edit_x = win->x + w;
	win->edit_w = win->w - w;
}

void set_window_coordinates(struct window *win, int x, int y)
{
	win->x = x;
	win->y = y;
	win->edit_x = x;
	win->edit_y = y + options.show_tab_bar;
}

void set_window_size(struct window *win, int w, int h)
{
	win->w = w;
	win->h = h;
	win->edit_w = w;
	win->edit_h = h - options.show_tab_bar - 1;

	calculate_line_numbers(win);
}
