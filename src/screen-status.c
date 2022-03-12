#include <stdint.h>
#include "screen.h"
#include "selection.h"
#include "util/debug.h"
#include "util/str-util.h"
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
    STATUS_OVERWRITE,
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
    ['o'] = STATUS_OVERWRITE,
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
    const EditorState *editor;
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

    if (f->editor->input_mode == INPUT_SEARCH) {
        SearchCaseSensitivity css = f->editor->options.case_sensitive_search;
        BUG_ON(css >= ARRAYLEN(css_strs));
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

static FormatSpecifierType lookup_format_specifier(unsigned char ch)
{
    if (unlikely(ch >= ARRAYLEN(format_specifiers))) {
        return STATUS_INVALID;
    }
    return format_specifiers[ch];
}

static void sf_format (
    const EditorState *e,
    const Window *w,
    char *buf,
    size_t size,
    const char *format
) {
    BUG_ON(size < 16);
    Formatter f = {
        .win = w,
        .editor = e,
        .buf = buf,
        .size = size - 5, // Max length of char and terminating NUL
    };

    const View *v = w->view;
    CodePoint u;

    while (f.pos < f.size && *format) {
        unsigned char ch = *format++;
        if (ch != '%') {
            add_separator(&f);
            add_ch(&f, ch);
            continue;
        }

        switch (lookup_format_specifier(*format++)) {
        case STATUS_BOM:
            if (v->buffer->bom) {
                add_status_literal(&f, "BOM");
            }
            break;
        case STATUS_FILENAME:
            add_status_str(&f, buffer_filename(v->buffer));
            break;
        case STATUS_MODIFIED:
            if (buffer_modified(v->buffer)) {
                add_separator(&f);
                add_ch(&f, '*');
            }
            break;
        case STATUS_READONLY:
            if (v->buffer->readonly) {
                add_status_literal(&f, "RO");
            } else if (v->buffer->temporary) {
                add_status_literal(&f, "TMP");
            }
            break;
        case STATUS_CURSOR_ROW:
            add_status_format(&f, "%ld", v->cy + 1);
            break;
        case STATUS_TOTAL_ROWS:
            add_status_format(&f, "%zu", v->buffer->nl);
            break;
        case STATUS_CURSOR_COL:
            add_status_format(&f, "%ld", v->cx_display + 1);
            break;
        case STATUS_CURSOR_COL_BYTES:
            add_status_format(&f, "%ld", v->cx_char + 1);
            if (v->cx_display != v->cx_char) {
                add_status_format(&f, "-%ld", v->cx_display + 1);
            }
            break;
        case STATUS_SCROLL_POSITION:
            add_status_pos(&f);
            break;
        case STATUS_ENCODING:
            add_status_str(&f, v->buffer->encoding.name);
            break;
        case STATUS_MISC:
            add_misc_status(&f);
            break;
        case STATUS_IS_CRLF:
            if (v->buffer->crlf_newlines) {
                add_status_literal(&f, "CRLF");
            }
            break;
        case STATUS_LINE_ENDING:
            if (v->buffer->crlf_newlines) {
                add_status_literal(&f, "CRLF");
            } else {
                add_status_literal(&f, "LF");
            }
            break;
        case STATUS_OVERWRITE:
            if (v->buffer->options.overwrite) {
                add_status_literal(&f, "OVR");
            } else {
                add_status_literal(&f, "INS");
            }
            break;
        case STATUS_SEPARATOR_LONG:
            f.separator = 3;
            break;
        case STATUS_SEPARATOR:
            f.separator = 1;
            break;
        case STATUS_FILETYPE:
            add_status_str(&f, v->buffer->options.filetype);
            break;
        case STATUS_UNICODE:
            if (unlikely(!block_iter_get_char(&v->cursor, &u))) {
                break;
            }
            if (u_is_unicode(u)) {
                add_status_format(&f, "U+%04X", u);
            } else {
                add_status_literal(&f, "Invalid");
            }
            break;
        case STATUS_ESCAPED_PERCENT:
            add_separator(&f);
            add_ch(&f, '%');
            break;
        case STATUS_INVALID:
        default:
            BUG("should be unreachable, due to validate_statusline_format()");
        }
    }

    f.buf[f.pos] = '\0';
}

UNITTEST {
    Buffer buffer = {
        .encoding = {.type = UTF8, .name = "UTF-8"},
        .options = {.filetype = "none"},
    };

    list_init(&buffer.blocks);
    Block *block = block_new(1);
    list_add_before(&block->node, &buffer.blocks);

    View view = {
        .buffer = &buffer,
        .cursor = {.head = &buffer.blocks, .blk = block},
    };

    Window window = {.view = &view};
    view.window = &window;

    EditorState e = {
        .input_mode = INPUT_NORMAL,
        .window = &window,
        .buffer = &buffer,
        .view = &view,
    };

    char buf[256];
    sf_format(&e, &window, buf, sizeof buf, "%% %n%s%y%s%Y%S%f%s%m%s%r... %E %t%S%N");
    BUG_ON(!streq(buf, "% LF 1 0   (No name) ... UTF-8 none"));

    sf_format(&e, &window, buf, sizeof buf, "%b%s%n%s%N%s%r%s%o");
    BUG_ON(!streq(buf, " LF INS"));

    buffer.bom = true;
    buffer.crlf_newlines = true;
    buffer.temporary = true;
    buffer.options.overwrite = true;
    sf_format(&e, &window, buf, sizeof buf, "%b%s%n%s%N%s%r%s%o");
    BUG_ON(!streq(buf, "BOM CRLF CRLF TMP OVR"));

    e.input_mode = INPUT_SEARCH;
    sf_format(&e, &window, buf, sizeof buf, "%M");
    BUG_ON(!streq(buf, "[case-sensitive = false]"));

    e.options.case_sensitive_search = CSS_AUTO;
    sf_format(&e, &window, buf, sizeof buf, "%M");
    BUG_ON(!streq(buf, "[case-sensitive = auto]"));

    for (unsigned char i = 0; i < ARRAYLEN(format_specifiers); i++) {
        FormatSpecifierType type =  format_specifiers[i];
        if (type == STATUS_INVALID) {
            continue;
        }
        const char fmt[4] = {'%', i, '\0'};
        sf_format(&e, &window, buf, sizeof(buf), fmt);
    }

    block_free(block);
}

void update_status_line(EditorState *e, const Window *win)
{
    char lbuf[256], rbuf[256];
    sf_format(e, win, lbuf, sizeof lbuf, e->options.statusline_left);
    sf_format(e, win, rbuf, sizeof rbuf, e->options.statusline_right);

    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;
    size_t lw = u_str_width(lbuf);
    size_t rw = u_str_width(rbuf);
    term_output_reset(term, win->x, win->w, 0);
    term_move_cursor(obuf, win->x, win->y + win->h - 1);
    set_builtin_color(e, BC_STATUSLINE);

    if (lw + rw <= win->w) {
        // Both fit
        term_add_str(obuf, lbuf);
        term_set_bytes(term, ' ', win->w - lw - rw);
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
        term_clear_eol(term);
    } else if (rw <= win->w) {
        // Right fits
        term_set_bytes(term, ' ', win->w - rw);
        term_add_str(obuf, rbuf);
    } else {
        term_clear_eol(term);
    }
}

size_t statusline_format_find_error(const char *str)
{
    for (size_t i = 0; str[i]; ) {
        if (str[i++] != '%') {
            continue;
        }
        if (lookup_format_specifier(str[i++]) == STATUS_INVALID) {
            return i - 1;
        }
    }
    return 0;
}
