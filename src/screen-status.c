#include "screen.h"
#include "editor.h"
#include "selection.h"
#include "terminal/output.h"
#include "util/debug.h"
#include "util/utf8.h"
#include "util/xsnprintf.h"

typedef struct {
    char *buf;
    size_t size;
    size_t pos;
    size_t separator;
    const Window *win;
    const char *misc_status;
} Formatter;

#define add_status_literal(f, s) add_status_bytes(f, s, STRLEN(s))

static void add_ch(Formatter *f, char ch)
{
    f->buf[f->pos++] = ch;
}

static void add_separator(Formatter *f)
{
    while (f->separator && f->pos < f->size) {
        add_ch(f, ' ');
        f->separator--;
    }
}

static void add_status_str(Formatter *f, const char *str)
{
    BUG_ON(!str);
    if (!str[0]) {
        return;
    }
    add_separator(f);
    size_t idx = 0;
    while (f->pos < f->size && str[idx]) {
        u_set_char(f->buf, &f->pos, u_str_get_char(str, &idx));
    }
}

static void add_status_bytes(Formatter *f, const char *str, size_t len)
{
    if (len == 0) {
        return;
    }
    add_separator(f);
    if (f->pos >= f->size) {
        return;
    }
    const size_t avail = f->size - f->pos;
    len = MIN(len, avail);
    memcpy(f->buf + f->pos, str, len);
    f->pos += len;
}

PRINTF(2)
static void add_status_format(Formatter *f, const char *format, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    size_t len = xvsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    add_status_bytes(f, buf, len);
}

static void add_status_pos(Formatter *f)
{
    size_t lines = f->win->view->buffer->nl;
    int h = f->win->edit_h;
    long pos = f->win->view->vy;
    if (lines <= h) {
        if (pos) {
            add_status_literal(f, "Bot");
        } else {
            add_status_literal(f, "All");
        }
    } else if (pos == 0) {
        add_status_literal(f, "Top");
    } else if (pos + h - 1 >= lines) {
        add_status_literal(f, "Bot");
    } else {
        const long d = lines - (h - 1);
        add_status_format(f, "%2ld%%", (pos * 100 + d / 2) / d);
    }
}

static void sf_format(Formatter *f, char *buf, size_t size, const char *format)
{
    View *v = f->win->view;

    f->buf = buf;
    f->size = size - 5; // Max length of char and terminating NUL
    f->pos = 0;
    f->separator = 0;

    CodePoint u;
    bool got_char = block_iter_get_char(&v->cursor, &u) > 0;
    while (f->pos < f->size && *format) {
        char ch = *format++;
        if (ch != '%') {
            add_separator(f);
            add_ch(f, ch);
            continue;
        }
        ch = *format++;
        switch (ch) {
        case 'b':
            if (v->buffer->bom) {
                add_status_literal(f, "BOM");
            }
            break;
        case 'f':
            add_status_str(f, buffer_filename(v->buffer));
            break;
        case 'm':
            if (buffer_modified(v->buffer)) {
                add_separator(f);
                add_ch(f, '*');
            }
            break;
        case 'r':
            if (v->buffer->readonly) {
                add_status_literal(f, "RO");
            } else if (v->buffer->temporary) {
                add_status_literal(f, "TMP");
            }
            break;
        case 'y':
            add_status_format(f, "%ld", v->cy + 1);
            break;
        case 'Y':
            add_status_format(f, "%zu", v->buffer->nl);
            break;
        case 'x':
            add_status_format(f, "%ld", v->cx_display + 1);
            break;
        case 'X':
            add_status_format(f, "%ld", v->cx_char + 1);
            if (v->cx_display != v->cx_char) {
                add_status_format(f, "-%ld", v->cx_display + 1);
            }
            break;
        case 'p':
            add_status_pos(f);
            break;
        case 'E':
            add_status_str(f, v->buffer->encoding.name);
            break;
        case 'M':
            if (f->misc_status) {
                add_status_str(f, f->misc_status);
            }
            break;
        case 'N':
            if (v->buffer->crlf_newlines) {
                add_status_literal(f, "CRLF");
            }
            break;
        case 'n':
            if (v->buffer->crlf_newlines) {
                add_status_literal(f, "CRLF");
            } else {
                add_status_literal(f, "LF");
            }
            break;
        case 'S':
            f->separator = 3;
            break;
        case 's':
            f->separator = 1;
            break;
        case 't':
            add_status_str(f, v->buffer->options.filetype);
            break;
        case 'u':
            if (got_char) {
                if (u_is_unicode(u)) {
                    add_status_format(f, "U+%04X", u);
                } else {
                    add_status_literal(f, "Invalid");
                }
            }
            break;
        case '%':
            add_separator(f);
            add_ch(f, '%');
            break;
        case '\0':
            f->buf[f->pos] = '\0';
            return;
        default:
            BUG("should be unreachable, due to validate_statusline_format()");
        }
    }
    f->buf[f->pos] = '\0';
}

static const char *format_misc_status(const Window *win)
{
    if (editor.input_mode == INPUT_SEARCH) {
        switch (editor.options.case_sensitive_search) {
        case CSS_FALSE:
            return "[case-sensitive = false]";
        case CSS_TRUE:
            return "[case-sensitive = true]";
        case CSS_AUTO:
            return "[case-sensitive = auto]";
        }
        BUG("unhandled case sensitivity type");
        return NULL;
    }

    if (win->view->selection == SELECT_NONE) {
        return NULL;
    }

    static char buf[sizeof("[n chars]") + DECIMAL_STR_MAX(size_t)];
    SelectionInfo si;
    init_selection(win->view, &si);

    switch (win->view->selection) {
    case SELECT_CHARS:
        xsnprintf(buf, sizeof(buf), "[%zu chars]", get_nr_selected_chars(&si));
        return buf;
    case SELECT_LINES:
        xsnprintf(buf, sizeof(buf), "[%zu lines]", get_nr_selected_lines(&si));
        return buf;
    case SELECT_NONE:
        // Already handled above
        break;
    }

    BUG("unhandled selection type");
    return NULL;
}

void update_status_line(const Window *win)
{
    Formatter f = {
        .win = win,
        .misc_status = format_misc_status(win)
    };
    char lbuf[256];
    char rbuf[256];
    sf_format(&f, lbuf, sizeof(lbuf), editor.options.statusline_left);
    sf_format(&f, rbuf, sizeof(rbuf), editor.options.statusline_right);

    term_output_reset(win->x, win->w, 0);
    term_move_cursor(win->x, win->y + win->h - 1);
    set_builtin_color(BC_STATUSLINE);
    size_t lw = u_str_width(lbuf);
    size_t rw = u_str_width(rbuf);
    if (lw + rw <= win->w) {
        // Both fit
        term_add_str(lbuf);
        term_set_bytes(' ', win->w - lw - rw);
        term_add_str(rbuf);
    } else if (lw <= win->w && rw <= win->w) {
        // Both would fit separately, draw overlapping
        term_add_str(lbuf);
        obuf.x = win->w - rw;
        term_move_cursor(win->x + win->w - rw, win->y + win->h - 1);
        term_add_str(rbuf);
    } else if (lw <= win->w) {
        // Left fits
        term_add_str(lbuf);
        term_clear_eol();
    } else if (rw <= win->w) {
        // Right fits
        term_set_bytes(' ', win->w - rw);
        term_add_str(rbuf);
    } else {
        term_clear_eol();
    }
}
