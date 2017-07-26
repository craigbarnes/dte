#include "format-status.h"
#include "window.h"
#include "view.h"
#include "uchar.h"

static void add_ch(struct formatter *f, char ch)
{
	f->buf[f->pos++] = ch;
}

static void add_separator(struct formatter *f)
{
	if (f->separator && f->pos < f->size)
		add_ch(f, ' ');
	f->separator = false;
}

static void add_status_str(struct formatter *f, const char *str)
{
	long idx = 0;

	if (!*str)
		return;

	add_separator(f);
	while (f->pos < f->size && str[idx])
		u_set_char(f->buf, &f->pos, u_str_get_char(str, &idx));
}

FORMAT(2)
static void add_status_format(struct formatter *f, const char *format, ...)
{
	char buf[1024];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	add_status_str(f, buf);
}

static void add_status_pos(struct formatter *f)
{
	long lines = f->win->view->buffer->nl;
	int h = f->win->edit_h;
	int pos = f->win->view->vy;

	if (lines <= h) {
		if (pos)
			add_status_str(f, "Bot");
		else
			add_status_str(f, "All");
	} else if (pos == 0) {
		add_status_str(f, "Top");
	} else if (pos + h - 1 >= lines) {
		add_status_str(f, "Bot");
	} else {
		int d = lines - (h - 1);
		add_status_format(f, "%2d%%", (pos * 100 + d / 2) / d);
	}
}

void sf_init(struct formatter *f, struct window *win)
{
	clear(f);
	f->win = win;
}

void sf_format(struct formatter *f, char *buf, long size, const char *format)
{
	struct view *v = f->win->view;
	bool got_char;
	unsigned int u;

	f->buf = buf;
	f->size = size - 5; // max length of char and terminating NUL
	f->pos = 0;
	f->separator = false;

	got_char = buffer_get_char(&v->cursor, &u) > 0;
	while (f->pos < f->size && *format) {
		char ch = *format++;
		if (ch != '%') {
			add_separator(f);
			add_ch(f, ch);
		} else {
			ch = *format++;
			switch (ch) {
			case 'f':
				add_status_str(f, buffer_filename(v->buffer));
				break;
			case 'm':
				if (buffer_modified(v->buffer))
					add_status_str(f, "*");
				break;
			case 'r':
				if (v->buffer->ro)
					add_status_str(f, "RO");
				break;
			case 'y':
				add_status_format(f, "%d", v->cy + 1);
				break;
			case 'Y':
				add_status_format(f, "%ld", v->buffer->nl);
				break;
			case 'x':
				add_status_format(f, "%d", v->cx_display + 1);
				break;
			case 'X':
				add_status_format(f, "%d", v->cx_char + 1);
				if (v->cx_display != v->cx_char)
					add_status_format(f, "-%d", v->cx_display + 1);
				break;
			case 'p':
				add_status_pos(f);
				break;
			case 'E':
				add_status_str(f, v->buffer->encoding);
				break;
			case 'M': {
				if (f->misc_status != NULL)
					add_status_str(f, f->misc_status);
				break;
			}
			case 'n':
				switch (v->buffer->newline) {
				case NEWLINE_UNIX:
					add_status_str(f, "LF");
					break;
				case NEWLINE_DOS:
					add_status_str(f, "CRLF");
					break;
				}
				break;
			case 's':
				f->separator = true;
				break;
			case 't':
				add_status_str(f, v->buffer->options.filetype);
				break;
			case 'u':
				if (got_char) {
					if (u_is_unicode(u)) {
						add_status_format(f, "U+%04X", u);
					} else {
						add_status_str(f, "Invalid");
					}
				}
				break;
			case '%':
				add_separator(f);
				add_ch(f, ch);
				break;
			}
		}
	}
	f->buf[f->pos] = 0;
}
