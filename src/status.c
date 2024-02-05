#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "status.h"
#include "search.h"
#include "selection.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/numtostr.h"
#include "util/utf8.h"
#include "util/xsnprintf.h"

typedef struct {
    char *buf;
    size_t size;
    size_t pos;
    size_t separator;
    const Window *window;
    const GlobalOptions *opts;
    const ModeHandler *mode;
} Formatter;

enum {
    SHORT_SEPARATOR_SIZE = 1, // For STATUS_SEPARATOR
    LONG_SEPARATOR_SIZE = 3, // For STATUS_SEPARATOR_LONG
    SEPARATOR_WRITE_SIZE = 4, // See add_separator()
};

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
    STATUS_INPUT_MODE,
    STATUS_MODIFIED,
    STATUS_LINE_ENDING,
    STATUS_OVERWRITE,
    STATUS_SCROLL_POSITION,
    STATUS_READONLY,
    STATUS_SEPARATOR,
    STATUS_FILETYPE,
    STATUS_UNICODE,
    STATUS_CURSOR_COL,
    STATUS_CURSOR_ROW,
} FormatSpecifierType;

static FormatSpecifierType lookup_format_specifier(unsigned char ch)
{
    switch (ch) {
    case '%': return STATUS_ESCAPED_PERCENT;
    case 'E': return STATUS_ENCODING;
    case 'M': return STATUS_MISC;
    case 'N': return STATUS_IS_CRLF;
    case 'S': return STATUS_SEPARATOR_LONG;
    case 'X': return STATUS_CURSOR_COL_BYTES;
    case 'Y': return STATUS_TOTAL_ROWS;
    case 'b': return STATUS_BOM;
    case 'f': return STATUS_FILENAME;
    case 'i': return STATUS_INPUT_MODE;
    case 'm': return STATUS_MODIFIED;
    case 'n': return STATUS_LINE_ENDING;
    case 'o': return STATUS_OVERWRITE;
    case 'p': return STATUS_SCROLL_POSITION;
    case 'r': return STATUS_READONLY;
    case 's': return STATUS_SEPARATOR;
    case 't': return STATUS_FILETYPE;
    case 'u': return STATUS_UNICODE;
    case 'x': return STATUS_CURSOR_COL;
    case 'y': return STATUS_CURSOR_ROW;
    }
    return STATUS_INVALID;
}

#define add_status_literal(f, s) add_status_bytes(f, s, STRLEN(s))

static void add_ch(Formatter *f, char ch)
{
    f->buf[f->pos++] = ch;
}

static void add_separator(Formatter *f)
{
    size_t len = MIN(f->separator, f->size - f->pos);
    BUG_ON(len > LONG_SEPARATOR_SIZE);
    char *dest = f->buf + f->pos;
    f->pos += len;
    f->separator = 0;

    // The extra buffer space set aside by sf_format() allows us to write
    // 4 bytes unconditionally here, to allow this memset() call to be
    // optimized into a simple move instruction
    memset(dest, ' ', SEPARATOR_WRITE_SIZE);
}

static void add_status_str(Formatter *f, const char *str)
{
    BUG_ON(!str);
    if (unlikely(!str[0])) {
        return;
    }

    add_separator(f);
    char *buf = f->buf;
    size_t pos = f->pos;

    for (size_t i = 0, size = f->size; pos < size && str[i]; ) {
        pos += u_set_char(buf + pos, u_str_get_char(str, &i));
    }

    f->pos = pos;
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

    size_t avail = f->size - f->pos;
    f->pos += copystrn(f->buf + f->pos, str, MIN(len, avail));
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

static void add_status_umax(Formatter *f, uintmax_t x)
{
    char buf[DECIMAL_STR_MAX(x)];
    size_t len = buf_umax_to_str(x, buf);
    add_status_bytes(f, buf, len);
}

static void add_status_bool(Formatter *f, bool state, const char *on, const char *off)
{
    const char *str = state ? on : off;
    add_status_bytes(f, str, strlen(str));
}

static void add_status_unicode(Formatter *f, const BlockIter *cursor)
{
    CodePoint u;
    if (unlikely(!block_iter_get_char(cursor, &u))) {
        return;
    }

    if (!u_is_unicode(u)) {
        add_status_literal(f, "Invalid");
        return;
    }

    char str[STRLEN("U+10FFFF") + 1];
    str[0] = 'U';
    str[1] = '+';
    size_t ndigits = buf_umax_to_hex_str(u, str + 2, 4);
    add_status_bytes(f, str, 2 + ndigits);
}

static void add_status_pos(Formatter *f)
{
    size_t lines = f->window->view->buffer->nl;
    int h = f->window->edit_h;
    long pos = f->window->view->vy;
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
        unsigned int d = lines - (h - 1);
        unsigned int percent = (pos * 100 + d / 2) / d;
        BUG_ON(percent > 100);
        char buf[4];
        size_t len = buf_uint_to_str(percent, buf);
        buf[len++] = '%';
        add_status_bytes(f, buf, len);
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

    if (f->mode->cmds == &search_mode_commands) {
        SearchCaseSensitivity css = f->opts->case_sensitive_search;
        BUG_ON(css >= ARRAYLEN(css_strs));
        add_status_bytes(f, css_strs[css].str, css_strs[css].len);
        return;
    }

    const View *view = f->window->view;
    if (view->selection == SELECT_NONE) {
        return;
    }

    SelectionInfo si;
    init_selection(view, &si);
    bool is_lines = (view->selection == SELECT_LINES);
    size_t n = is_lines ? get_nr_selected_lines(&si) : get_nr_selected_chars(&si);
    const char *unit = is_lines ? "line" : "char";
    const char *plural = unlikely(n == 1) ? "" : "s";
    add_status_format(f, "[%zu %s%s]", n, unit, plural);
}

static void expand_format_specifier(Formatter *f, FormatSpecifierType type)
{
    const View *view = f->window->view;
    const Buffer *buffer = view->buffer;
    switch (type) {
    case STATUS_BOM:
        if (buffer->bom) {
            add_status_literal(f, "BOM");
        }
        return;
    case STATUS_FILENAME:
        add_status_str(f, buffer_filename(buffer));
        return;
    case STATUS_INPUT_MODE:
        add_status_str(f, f->mode->name);
        return;
    case STATUS_MODIFIED:
        if (buffer_modified(buffer)) {
            add_separator(f);
            add_ch(f, '*');
        }
        return;
    case STATUS_READONLY:
        if (buffer->readonly) {
            add_status_literal(f, "RO");
        } else if (buffer->temporary) {
            add_status_literal(f, "TMP");
        }
        return;
    case STATUS_CURSOR_ROW:
        add_status_umax(f, view->cy + 1);
        return;
    case STATUS_TOTAL_ROWS:
        add_status_umax(f, buffer->nl);
        return;
    case STATUS_CURSOR_COL:
        add_status_umax(f, view->cx_display + 1);
        return;
    case STATUS_CURSOR_COL_BYTES:
        add_status_umax(f, view->cx_char + 1);
        if (view->cx_display != view->cx_char) {
            add_ch(f, '-');
            add_status_umax(f, view->cx_display + 1);
        }
        return;
    case STATUS_SCROLL_POSITION:
        add_status_pos(f);
        return;
    case STATUS_ENCODING:
        add_status_str(f, buffer->encoding);
        return;
    case STATUS_MISC:
        add_misc_status(f);
        return;
    case STATUS_IS_CRLF:
        if (buffer->crlf_newlines) {
            add_status_literal(f, "CRLF");
        }
        return;
    case STATUS_LINE_ENDING:
        add_status_bool(f, buffer->crlf_newlines, "CRLF", "LF");
        return;
    case STATUS_OVERWRITE:
        add_status_bool(f, buffer->options.overwrite, "OVR", "INS");
        return;
    case STATUS_SEPARATOR_LONG:
        f->separator = LONG_SEPARATOR_SIZE;
        return;
    case STATUS_SEPARATOR:
        f->separator = SHORT_SEPARATOR_SIZE;
        return;
    case STATUS_FILETYPE:
        add_status_str(f, buffer->options.filetype);
        return;
    case STATUS_UNICODE:
        add_status_unicode(f, &view->cursor);
        return;
    case STATUS_ESCAPED_PERCENT:
        add_separator(f);
        add_ch(f, '%');
        return;
    case STATUS_INVALID:
        break;
    }

    BUG("should be unreachable, due to validate_statusline_format()");
}

size_t sf_format (
    const Window *window,
    const GlobalOptions *opts,
    const ModeHandler *mode,
    char *buf, // NOLINT(readability-non-const-parameter)
    size_t size,
    const char *format
) {
    BUG_ON(size < 16);
    Formatter f = {
        .window = window,
        .opts = opts,
        .mode = mode,
        .buf = buf,
        .size = size - SEPARATOR_WRITE_SIZE - UTF8_MAX_SEQ_LEN - 1,
    };

    while (f.pos < f.size && *format) {
        unsigned char ch = *format++;
        if (ch != '%') {
            add_separator(&f);
            add_ch(&f, ch);
            continue;
        }
        FormatSpecifierType type = lookup_format_specifier(*format++);
        expand_format_specifier(&f, type);
    }

    f.buf[f.pos] = '\0';
    return u_str_width(f.buf);
}

// Returns the offset of the first invalid format specifier, or 0 if
// the whole format string is valid. It's safe to use 0 to indicate
// "no errors", since it's not possible for there to be an error at
// the very start of the string.
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
