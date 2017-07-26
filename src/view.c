#include "view.h"
#include "window.h"
#include "uchar.h"

struct view *view;

void view_update_cursor_y(struct view *v)
{
	struct buffer *b = v->buffer;
	struct block *blk;
	unsigned int nl = 0;

	list_for_each_entry(blk, &b->blocks, node) {
		if (blk == v->cursor.blk) {
			nl += count_nl(blk->data, v->cursor.offset);
			v->cy = nl;
			return;
		}
		nl += blk->nl;
	}
	BUG_ON(1);
}

void view_update_cursor_x(struct view *v)
{
	unsigned int tw = v->buffer->options.tab_width;
	long idx = 0;
	struct lineref lr;
	int c = 0;
	int w = 0;

	v->cx = fetch_this_line(&v->cursor, &lr);
	while (idx < v->cx) {
		unsigned int u = lr.line[idx++];

		c++;
		if (likely(u < 0x80)) {
			if (!u_is_ctrl(u)) {
				w++;
			} else if (u == '\t') {
				w = (w + tw) / tw * tw;
			} else {
				w += 2;
			}
		} else {
			idx--;
			u = u_get_nonascii(lr.line, lr.size, &idx);
			w += u_char_width(u);
		}
	}
	v->cx_char = c;
	v->cx_display = w;
}

static int view_is_cursor_visible(struct view *v)
{
	return v->cy < v->vy || v->cy > v->vy + v->window->edit_h - 1;
}

static void view_center_to_cursor(struct view *v)
{
	long lines = v->buffer->nl;
	struct window *w = v->window;
	unsigned int hh = w->edit_h / 2;

	if (w->edit_h >= lines || v->cy < hh) {
		v->vy = 0;
		return;
	}

	v->vy = v->cy - hh;
	if (v->vy + w->edit_h > lines) {
		/* -1 makes one ~ line visible so that you know where the EOF is */
		v->vy -= v->vy + w->edit_h - lines - 1;
	}
}

static void view_update_vx(struct view *v)
{
	struct window *w = v->window;
	unsigned int c = 8;

	if (v->cx_display - v->vx >= w->edit_w)
		v->vx = (v->cx_display - w->edit_w + c) / c * c;
	if (v->cx_display < v->vx)
		v->vx = v->cx_display / c * c;
}

static void view_update_vy(struct view *v)
{
	struct window *w = v->window;
	int margin = window_get_scroll_margin(w);
	int max_y = v->vy + w->edit_h - 1 - margin;

	if (v->cy < v->vy + margin) {
		v->vy = v->cy - margin;
		if (v->vy < 0)
			v->vy = 0;
	} else if (v->cy > max_y) {
		v->vy += v->cy - max_y;
		max_y = v->buffer->nl - w->edit_h + 1;
		if (v->vy > max_y && max_y >= 0)
			v->vy = max_y;
	}
}

void view_update(struct view *v)
{
	view_update_vx(v);
	if (v->force_center || (v->center_on_scroll && view_is_cursor_visible(v))) {
		view_center_to_cursor(v);
	} else {
		view_update_vy(v);
	}
	v->force_center = false;
	v->center_on_scroll = false;
}

int view_get_preferred_x(struct view *v)
{
	if (v->preferred_x < 0) {
		view_update_cursor_x(v);
		v->preferred_x = v->cx_display;
	}
	return v->preferred_x;
}

bool view_can_close(struct view *v)
{
	if (!buffer_modified(v->buffer)) {
		return true;
	}
	// open in another window?
	return v->buffer->views.count > 1;
}

char *view_get_word_under_cursor(struct view *v)
{
	struct lineref lr;
	long i, ei, si = fetch_this_line(&v->cursor, &lr);

	while (si < lr.size) {
		i = si;
		if (u_is_word_char(u_get_char(lr.line, lr.size, &i)))
			break;
		si = i;
	}
	if (si == lr.size)
		return NULL;

	ei = si;
	while (si > 0) {
		i = si;
		if (!u_is_word_char(u_prev_char(lr.line, &i)))
			break;
		si = i;
	}
	while (ei < lr.size) {
		i = ei;
		if (!u_is_word_char(u_get_char(lr.line, lr.size, &i)))
			break;
		ei = i;
	}
	return xstrslice(lr.line, si, ei);
}
