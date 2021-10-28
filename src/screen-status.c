#include <stdint.h>
#include "screen.h"
#include "editor.h"
#include "selection.h"
#include "util/debug.h"
#include "util/utf8.h"
#include "util/xsnprintf.h"

typedef enum {
    STATUS_INVALID = 0,
    STATUS_ESCAPED_PERCENT,
    STATUS_ENCODING,
    STATUS_MISC,
    STATUS_IS_CRLF,
    STATUS_SEPARATOR_LONG,
    STATUS_CURSOR_COL_BYTES,
    STATUS_TOTAL_ROWS,
    STATUS_BOM,
    STATUS_FILENAME,
    STATUS_MODIFIED,
    STATUS_LINE_ENDING,
    STATUS_SCROLL_POSITION,
    STATUS_READONLY,
    STATUS_SEPARATOR,
    STATUS_FILETYPE,
    STATUS_UNICODE,
    STATUS_CURSOR_COL,
    STATUS_CURSOR_ROW,
} FormatSpecifierType;

static const uint8_t format_specifiers[] = {
    ['%'] = STATUS_ESCAPED_PERCENT,
    ['E'] = STATUS_ENCODING,
    ['M'] = STATUS_MISC,
    ['N'] = STATUS_IS_CRLF,
    ['S'] = STATUS_SEPARATOR_LONG,
    ['X'] = STATUS_CURSOR_COL_BYTES,
    ['Y'] = STATUS_TOTAL_ROWS,
    ['b'] = STATUS_BOM,
    ['f'] = STATUS_FILENAME,
    ['m'] = STATUS_MODIFIED,
    ['n'] = STATUS_LINE_ENDING,
    ['p'] = STATUS_SCROLL_POSITION,
    ['r'] = STATUS_READONLY,
    ['s'] = STATUS_SEPARATOR,
    ['t'] = STATUS_FILETYPE,
    ['u'] = STATUS_UNICODE,
    ['x'] = STATUS_CURSOR_COL,
    ['y'] = STATUS_CURSOR_ROW,
};

typedef struct {
    char *buf;
    size_t size;
    size_t pos;
    size_t separator;
    const Window *win;
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
    if (unlikely(!str[0])) {
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
    if (unlikely(len == 0)) {
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

static void add_misc_status(Formatter *f)
{
    static const struct {
        const char str[24];
        size_t len;
    } css_strs[] = {
        [CSS_FALSE] = {STRN("[case-sensitive = false]")},
        [CSS_TRUE] = {STRN("[case-sensitive = true]")},
        [CSS_AUTO] = {STRN("[case-sensitive = auto]")},
    };

    if (editor.input_mode == INPUT_SEARCH) {
        SearchCaseSensitivity css = editor.options.case_sensitive_search;
        BUG_ON(css >= ARRAY_COUNT(css_strs));
        add_status_bytes(f, css_strs[css].str, css_strs[css].len);
        return;
    }

    const View *v = f->win->view;
    if (v->selection == SELECT_NONE) {
        return;
    }

    SelectionInfo si;
    init_selection(v, &si);

    switch (v->selection) {
    case SELECT_CHARS:
        add_status_format(f, "[%zu chars]", get_nr_selected_chars(&si));
        return;
    case SELECT_LINES:
        add_status_format(f, "[%zu lines]", get_nr_selected_lines(&si));
        return;
    case SELECT_NONE:
        // Already handled above
        break;
    }

    BUG("unhandled selection type");
}

static void sf_format(Formatter *f, char *buf, size_t size, const char *format)
{
    f->buf = buf;
    f->size = size - 5; // Max length of char and terminating NUL
    f->pos = 0;
    f->separator = 0;

    View *v = f->win->view;
    CodePoint u;

    while (f->pos < f->size && *format) {
        unsigned char ch = *format++;
        if (ch != '%') {
            add_separator(f);
            add_ch(f, ch);
            continue;
        }

        ch = *format++;
        BUG_ON(ch >= ARRAY_COUNT(format_specifiers));

        // Note: this explicit type conversion (from uint8_t) is here to
        // ensure "-Wswitch-enum" warnings are in effect
        FormatSpecifierType type = format_specifiers[ch];

        switch (type) {
        case STATUS_BOM:
            if (v->buffer->bom) {
                add_status_literal(f, "BOM");
            }
            break;
        case STATUS_FILENAME:
            add_status_str(f, buffer_filename(v->buffer));
            break;
        case STATUS_MODIFIED:
            if (buffer_modified(v->buffer)) {
                add_separator(f);
                add_ch(f, '*');
            }
            break;
        case STATUS_READONLY:
            if (v->buffer->readonly) {
                add_status_literal(f, "RO");
            } else if (v->buffer->temporary) {
                add_status_literal(f, "TMP");
            }
            break;
        case STATUS_CURSOR_ROW:
            add_status_format(f, "%ld", v->cy + 1);
            break;
        case STATUS_TOTAL_ROWS:
            add_status_format(f, "%zu", v->buffer->nl);
            break;
        case STATUS_CURSOR_COL:
            add_status_format(f, "%ld", v->cx_display + 1);
            break;
        case STATUS_CURSOR_COL_BYTES:
            add_status_format(f, "%ld", v->cx_char + 1);
            if (v->cx_display != v->cx_char) {
                add_status_format(f, "-%ld", v->cx_display + 1);
            }
            break;
        case STATUS_SCROLL_POSITION:
            add_status_pos(f);
            break;
        case STATUS_ENCODING:
            add_status_str(f, v->buffer->encoding.name);
            break;
        case STATUS_MISC:
            add_misc_status(f);
            break;
        case STATUS_IS_CRLF:
            if (v->buffer->crlf_newlines) {
                add_status_literal(f, "CRLF");
            }
            break;
        case STATUS_LINE_ENDING:
            if (v->buffer->crlf_newlines) {
                add_status_literal(f, "CRLF");
            } else {
                add_status_literal(f, "LF");
            }
            break;
        case STATUS_SEPARATOR_LONG:
            f->separator = 3;
            break;
        case STATUS_SEPARATOR:
            f->separator = 1;
            break;
        case STATUS_FILETYPE:
            add_status_str(f, v->buffer->options.filetype);
            break;
        case STATUS_UNICODE:
            if (unlikely(!block_iter_get_char(&v->cursor, &u))) {
                break;
            }
            if (u_is_unicode(u)) {
                add_status_format(f, "U+%04X", u);
            } else {
                add_status_literal(f, "Invalid");
            }
            break;
        case STATUS_ESCAPED_PERCENT:
            add_separator(f);
            add_ch(f, '%');
            break;
        case STATUS_INVALID:
        default:
            BUG("should be unreachable, due to validate_statusline_format()");
        }
    }

    f->buf[f->pos] = '\0';
}

void update_status_line(TermOutputBuffer *obuf, const Window *win)
{
    Formatter f = {.win = win};
    char lbuf[256];
    char rbuf[256];
    sf_format(&f, lbuf, sizeof(lbuf), editor.options.statusline_left);
    sf_format(&f, rbuf, sizeof(rbuf), editor.options.statusline_right);

    term_output_reset(obuf, win->x, win->w, 0);
    term_move_cursor(obuf, win->x, win->y + win->h - 1);
    set_builtin_color(obuf, BC_STATUSLINE);
    size_t lw = u_str_width(lbuf);
    size_t rw = u_str_width(rbuf);
    if (lw + rw <= win->w) {
        // Both fit
        term_add_str(obuf, lbuf);
        term_set_bytes(obuf, ' ', win->w - lw - rw);
        term_add_str(obuf, rbuf);
    } else if (lw <= win->w && rw <= win->w) {
        // Both would fit separately, draw overlapping
        term_add_str(obuf, lbuf);
        obuf->x = win->w - rw;
        term_move_cursor(obuf, win->x + win->w - rw, win->y + win->h - 1);
        term_add_str(obuf, rbuf);
    } else if (lw <= win->w) {
        // Left fits
        term_add_str(obuf, lbuf);
        term_clear_eol(obuf);
    } else if (rw <= win->w) {
        // Right fits
        term_set_bytes(obuf, ' ', win->w - rw);
        term_add_str(obuf, rbuf);
    } else {
        term_clear_eol(obuf);
    }
}

size_t statusline_format_find_error(const char *str)
{
    size_t i = 0;
    while (str[i]) {
        unsigned char c = str[i++];
        if (c != '%') {
            continue;
        }
        c = str[i++];
        if (c >= ARRAY_COUNT(format_specifiers) || !format_specifiers[c]) {
            return i - 1;
        }
    }
    return 0;
}
