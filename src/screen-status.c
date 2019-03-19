#include "screen.h"
#include "editor.h"
#include "selection.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "util/uchar.h"
#include "util/xsnprintf.h"

typedef struct {
    char *buf;
    size_t size;
    size_t pos;
    bool separator;
    Window *win;
    const char *misc_status;
} Formatter;

static void add_ch(Formatter *f, char ch)
{
    f->buf[f->pos++] = ch;
}

static void add_separator(Formatter *f)
{
    if (f->separator && f->pos < f->size) {
        add_ch(f, ' ');
    }
    f->separator = false;
}

static void add_status_str(Formatter *f, const char *str)
{
    if (!*str) {
        return;
    }
    add_separator(f);
    size_t idx = 0;
    while (f->pos < f->size && str[idx]) {
        u_set_char(f->buf, &f->pos, u_str_get_char(str, &idx));
    }
}

PRINTF(2)
static void add_status_format(Formatter *f, const char *format, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    xvsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    add_status_str(f, buf);
}

static void add_status_pos(Formatter *f)
{
    size_t lines = f->win->view->buffer->nl;
    int h = f->win->edit_h;
    int pos = f->win->view->vy;

    if (lines <= h) {
        if (pos) {
            add_status_str(f, "Bot");
        } else {
            add_status_str(f, "All");
        }
    } else if (pos == 0) {
        add_status_str(f, "Top");
    } else if (pos + h - 1 >= lines) {
        add_status_str(f, "Bot");
    } else {
        int d = lines - (h - 1);
        add_status_format(f, "%2d%%", (pos * 100 + d / 2) / d);
    }
}

static void sf_format(Formatter *f, char *buf, size_t size, const char *format)
{
    View *v = f->win->view;

    f->buf = buf;
    f->size = size - 5; // Max length of char and terminating NUL
    f->pos = 0;
    f->separator = false;

    CodePoint u;
    bool got_char = buffer_get_char(&v->cursor, &u) > 0;
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
                if (buffer_modified(v->buffer)) {
                    add_status_str(f, "*");
                }
                break;
            case 'r':
                if (v->buffer->readonly) {
                    add_status_str(f, "RO");
                }
                break;
            case 'y':
                add_status_format(f, "%d", v->cy + 1);
                break;
            case 'Y':
                add_status_format(f, "%zu", v->buffer->nl);
                break;
            case 'x':
                add_status_format(f, "%d", v->cx_display + 1);
                break;
            case 'X':
                add_status_format(f, "%d", v->cx_char + 1);
                if (v->cx_display != v->cx_char) {
                    add_status_format(f, "-%d", v->cx_display + 1);
                }
                break;
            case 'p':
                add_status_pos(f);
                break;
            case 'E':
                add_status_str(f, v->buffer->encoding);
                break;
            case 'M': {
                if (f->misc_status != NULL) {
                    add_status_str(f, f->misc_status);
                }
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
            case '\0':
                f->buf[f->pos] = '\0';
                return;
            }
        }
    }
    f->buf[f->pos] = '\0';
}

static const char *format_misc_status(Window *win)
{
    static char misc_status[32] = {'\0'};

    if (editor.input_mode == INPUT_SEARCH) {
        xsnprintf (
            misc_status,
            sizeof(misc_status),
            "[case-sensitive = %s]",
            case_sensitivity_to_string(editor.options.case_sensitive_search)
        );
    } else if (win->view->selection) {
        SelectionInfo info;
        init_selection(win->view, &info);
        if (win->view->selection == SELECT_LINES) {
            xsnprintf (
                misc_status,
                sizeof(misc_status),
                "[%zu lines]",
                get_nr_selected_lines(&info)
            );
        } else {
            xsnprintf (
                misc_status,
                sizeof(misc_status),
                "[%zu chars]",
                get_nr_selected_chars(&info)
            );
        }
    } else {
        return NULL;
    }
    return misc_status;
}

void update_status_line(Window *win)
{
    Formatter f = {.win = win};
    char lbuf[256];
    char rbuf[256];
    f.misc_status = format_misc_status(win);
    sf_format(&f, lbuf, sizeof(lbuf), editor.options.statusline_left);
    sf_format(&f, rbuf, sizeof(rbuf), editor.options.statusline_right);

    buf_reset(win->x, win->w, 0);
    terminal.move_cursor(win->x, win->y + win->h - 1);
    set_builtin_color(BC_STATUSLINE);
    size_t lw = u_str_width(lbuf);
    size_t rw = u_str_width(rbuf);
    if (lw + rw <= win->w) {
        // Both fit
        buf_add_str(lbuf);
        buf_set_bytes(' ', win->w - lw - rw);
        buf_add_str(rbuf);
    } else if (lw <= win->w && rw <= win->w) {
        // Both would fit separately, draw overlapping
        buf_add_str(lbuf);
        obuf.x = win->w - rw;
        terminal.move_cursor(win->x + win->w - rw, win->y + win->h - 1);
        buf_add_str(rbuf);
    } else if (lw <= win->w) {
        // Left fits
        buf_add_str(lbuf);
        buf_clear_eol();
    } else if (rw <= win->w) {
        // Right fits
        buf_set_bytes(' ', win->w - rw);
        buf_add_str(rbuf);
    } else {
        buf_clear_eol();
    }
}
