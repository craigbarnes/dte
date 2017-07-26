#include "screen.h"
#include "format-status.h"
#include "editor.h"
#include "view.h"
#include "obuf.h"
#include "cmdline.h"
#include "search.h"
#include "uchar.h"
#include "frame.h"
#include "git-open.h"
#include "path.h"
#include "input-special.h"
#include "selection.h"

void set_color(struct term_color *color)
{
	struct term_color tmp = *color;

	// NOTE: -2 (keep) is treated as -1 (default)
	if (tmp.fg  < 0)
		tmp.fg = builtin_colors[BC_DEFAULT]->fg;
	if (tmp.bg  < 0)
		tmp.bg = builtin_colors[BC_DEFAULT]->bg;
	buf_set_color(&tmp);
}

void set_builtin_color(enum builtin_color c)
{
	set_color(builtin_colors[c]);
}

static const char *format_misc_status(struct window *win)
{
	static char misc_status[32];

	if (special_input_misc_status(misc_status))
		return misc_status;

	if (input_mode == INPUT_SEARCH) {
		snprintf(misc_status, sizeof(misc_status), "[case-sensitive = %s]",
			case_sensitive_search_enum[options.case_sensitive_search]);
	} else if (win->view->selection) {
		struct selection_info info;

		init_selection(win->view, &info);
		if (win->view->selection == SELECT_LINES) {
			snprintf(misc_status, sizeof(misc_status), "[%d lines]", get_nr_selected_lines(&info));
		} else {
			snprintf(misc_status, sizeof(misc_status), "[%d chars]", get_nr_selected_chars(&info));
		}
	} else {
		return NULL;
	}
	return misc_status;
}

void update_status_line(struct window *win)
{
	struct formatter f;
	char lbuf[256];
	char rbuf[256];
	int lw, rw;

	sf_init(&f, win);
	f.misc_status = format_misc_status(win);
	sf_format(&f, lbuf, sizeof(lbuf), options.statusline_left);
	sf_format(&f, rbuf, sizeof(rbuf), options.statusline_right);

	buf_reset(win->x, win->w, 0);
	buf_move_cursor(win->x, win->y + win->h - 1);
	set_builtin_color(BC_STATUSLINE);
	lw = u_str_width(lbuf);
	rw = u_str_width(rbuf);
	if (lw + rw <= win->w) {
		// both fit
		buf_add_str(lbuf);
		buf_set_bytes(' ', win->w - lw - rw);
		buf_add_str(rbuf);
	} else if (lw <= win->w && rw <= win->w) {
		// both would fit separately, draw overlapping
		buf_add_str(lbuf);
		obuf.x = win->w - rw;
		buf_move_cursor(win->x + win->w - rw, win->y + win->h - 1);
		buf_add_str(rbuf);
	} else if (lw <= win->w) {
		// left fits
		buf_add_str(lbuf);
		buf_clear_eol();
	} else if (rw <= win->w) {
		// right fits
		buf_set_bytes(' ', win->w - rw);
		buf_add_str(rbuf);
	} else {
		buf_clear_eol();
	}
}

int print_command(char prefix)
{
	long i, w;
	unsigned int u;
	int x;

	// width of characters up to and including cursor position
	w = 1; // ":" (prefix)
	i = 0;
	while (i <= cmdline.pos && i < cmdline.buf.len) {
		u = u_get_char(cmdline.buf.buffer, cmdline.buf.len, &i);
		w += u_char_width(u);
	}
	if (cmdline.pos == cmdline.buf.len) {
		w++;
	}
	if (w > screen_w)
		obuf.scroll_x = w - screen_w;

	set_builtin_color(BC_COMMANDLINE);
	i = 0;
	buf_put_char(prefix);
	x = obuf.x - obuf.scroll_x;
	while (i < cmdline.buf.len) {
		BUG_ON(obuf.x > obuf.scroll_x + obuf.width);
		u = u_get_char(cmdline.buf.buffer, cmdline.buf.len, &i);
		if (!buf_put_char(u))
			break;
		if (i <= cmdline.pos)
			x = obuf.x - obuf.scroll_x;
	}
	return x;
}

void print_message(const char *msg, bool is_error)
{
	enum builtin_color c = BC_COMMANDLINE;
	long i = 0;

	if (msg[0])
		c = is_error ? BC_ERRORMSG : BC_INFOMSG;
	set_builtin_color(c);
	while (msg[i]) {
		unsigned int u = u_get_char(msg, i + 4, &i);
		if (!buf_put_char(u))
			break;
	}
}

void update_term_title(struct buffer *b)
{
	static int term_type = -1;
	char title[1024];

	if (term_type == -1) {
		const char *term = getenv("TERM");

		term_type = 0;
		if (term) {
			if (strstr(term, "xterm") || strstr(term, "rxvt")) {
				term_type = 1;
			} else if (streq(term, "screen")) {
				term_type = 2;
			}
		}
	}

	// FIXME: title must not contain control characters
	snprintf(title, sizeof(title), "%s %c %s",
		buffer_filename(b),
		buffer_modified(b) ? '+' : '-',
		program);

	switch (term_type) {
	case 1:
		// xterm or compatible
		buf_escape("\033]2;");
		buf_escape(title);
		buf_escape("\007");
		break;
	case 2:
		// tmux or screen
		// NOTE: screen might need to be configured to get it working
		buf_escape("\033_");
		buf_escape(title);
		buf_escape("\033\\");
		break;
	}
}

void mask_color(struct term_color *color, const struct term_color *over)
{
	if (over->fg != -2)
		color->fg = over->fg;
	if (over->bg != -2)
		color->bg = over->bg;
	if (!(over->attr & ATTR_KEEP))
		color->attr = over->attr;
}

static void print_separator(struct window *win)
{
	int y;

	if (win->x + win->w == screen_w)
		return;

	for (y = 0; y < win->h; y++) {
		buf_move_cursor(win->x + win->w, win->y + y);
		buf_add_ch('|');
	}
}

void update_separators(void)
{
	set_builtin_color(BC_STATUSLINE);
	for_each_window(print_separator);
}

void update_line_numbers(struct window *win, bool force)
{
	struct view *v = win->view;
	long lines = v->buffer->nl;
	int i, first, last;
	int x = win->x + vertical_tabbar_width(win);

	calculate_line_numbers(win);

	first = v->vy + 1;
	last = v->vy + win->edit_h;
	if (last > lines)
		last = lines;

	if (!force && win->line_numbers.first == first && win->line_numbers.last == last)
		return;

	win->line_numbers.first = first;
	win->line_numbers.last = last;

	buf_reset(win->x, win->w, 0);
	set_builtin_color(BC_LINENUMBER);
	for (i = 0; i < win->edit_h; i++) {
		int line = v->vy + i + 1;
		int w = win->line_numbers.width - 1;
		char buf[32];

		if (line > lines) {
			snprintf(buf, sizeof(buf), "%*s ", w, "");
		} else {
			snprintf(buf, sizeof(buf), "%*d ", w, line);
		}
		buf_move_cursor(x, win->edit_y + i);
		buf_add_bytes(buf, win->line_numbers.width);
	}
}

void update_git_open(void)
{
	int x = 0;
	int y = 0;
	int w = screen_w;
	int h = screen_h - 1;
	int max_y = git_open.scroll + h - 1;
	int i;

	if (h >= git_open.files.count)
		git_open.scroll = 0;
	if (git_open.scroll > git_open.selected)
		git_open.scroll = git_open.selected;
	if (git_open.selected > max_y)
		git_open.scroll += git_open.selected - max_y;

	buf_reset(x, w, 0);
	buf_move_cursor(0, 0);
	cmdline_x = print_command('/');
	buf_clear_eol();
	y++;

	for (i = 0; i < h; i++) {
		int file_idx = git_open.scroll + i;
		char *file;
		struct term_color color;

		if (file_idx >= git_open.files.count)
			break;

		file = git_open.files.ptrs[file_idx];
		obuf.x = 0;
		buf_move_cursor(x, y + i);

		color = *builtin_colors[BC_DEFAULT];
		if (file_idx == git_open.selected)
			mask_color(&color, builtin_colors[BC_SELECTION]);
		buf_set_color(&color);
		buf_add_str(file);
		buf_clear_eol();
	}
	set_builtin_color(BC_DEFAULT);
	for (; i < h; i++) {
		obuf.x = 0;
		buf_move_cursor(x, y + i);
		buf_clear_eol();
	}
}

void update_window_sizes(void)
{
	set_frame_size(root_frame, screen_w, screen_h - 1);
	update_window_coordinates();
}

void update_screen_size(void)
{
	if (!term_get_size(&screen_w, &screen_h)) {
		if (screen_w < 3)
			screen_w = 3;
		if (screen_h < 3)
			screen_h = 3;
		update_window_sizes();
	}
}
