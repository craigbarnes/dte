#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "output.h"
#include "terminal.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/utf8.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

void term_output_init(TermOutputBuffer *obuf)
{
    *obuf = (TermOutputBuffer) {
        .buf = xmalloc(TERM_OUTBUF_SIZE)
    };
}

void term_output_free(TermOutputBuffer *obuf)
{
    free(obuf->buf);
}

static void obuf_need_space(TermOutputBuffer *obuf, size_t count)
{
    if (unlikely(obuf_avail(obuf) < count)) {
        term_output_flush(obuf);
    }
}

void term_output_reset(Terminal *term, size_t start_x, size_t width, size_t scroll_x)
{
    TermOutputBuffer *obuf = &term->obuf;
    obuf->x = 0;
    obuf->width = width;
    obuf->scroll_x = scroll_x;
    obuf->tab_width = 8;
    obuf->tab = TAB_CONTROL;
    obuf->can_clear = start_x + width == term->width;
}

// Does not update obuf.x
void term_add_bytes(TermOutputBuffer *obuf, const char *str, size_t count)
{
    if (unlikely(count > obuf_avail(obuf))) {
        term_output_flush(obuf);
        if (unlikely(count >= TERM_OUTBUF_SIZE)) {
            (void)!xwrite(STDOUT_FILENO, str, count);
            return;
        }
    }
    memcpy(obuf->buf + obuf->count, str, count);
    obuf->count += count;
}

void term_repeat_byte(TermOutputBuffer *obuf, char ch, size_t count)
{
    while (count) {
        obuf_need_space(obuf, 1);
        size_t avail = obuf_avail(obuf);
        size_t n = MIN(count, avail);
        memset(obuf->buf + obuf->count, ch, n);
        obuf->count += n;
        count -= n;
    }
}

static void ecma48_repeat_byte(TermOutputBuffer *obuf, char ch, size_t count)
{
    if (!ascii_isprint(ch) || count < 6 || count > 30000) {
        term_repeat_byte(obuf, ch, count);
        return;
    }
    term_add_byte(obuf, ch);
    term_add_literal(obuf, "\033[");
    term_add_uint(obuf, count - 1);
    term_add_byte(obuf, 'b');
}

void term_set_bytes(Terminal *term, char ch, size_t count)
{
    TermOutputBuffer *obuf = &term->obuf;
    if (obuf->x + count > obuf->scroll_x + obuf->width) {
        count = obuf->scroll_x + obuf->width - obuf->x;
    }
    ssize_t skip = obuf->scroll_x - obuf->x;
    if (skip > 0) {
        if (skip > count) {
            skip = count;
        }
        obuf->x += skip;
        count -= skip;
    }
    obuf->x += count;
    if (term->features & TFLAG_ECMA48_REPEAT) {
        ecma48_repeat_byte(obuf, ch, count);
    } else {
        term_repeat_byte(obuf, ch, count);
    }
}

// Does not update obuf.x
void term_add_byte(TermOutputBuffer *obuf, char ch)
{
    obuf_need_space(obuf, 1);
    obuf->buf[obuf->count++] = ch;
}

void term_add_strview(TermOutputBuffer *obuf, StringView sv)
{
    if (sv.length) {
        term_add_bytes(obuf, sv.data, sv.length);
    }
}

void term_add_str(TermOutputBuffer *obuf, const char *str)
{
    size_t i = 0;
    while (str[i]) {
        if (!term_put_char(obuf, u_str_get_char(str, &i))) {
            break;
        }
    }
}

// Does not update obuf.x
void term_add_uint(TermOutputBuffer *obuf, unsigned int x)
{
    obuf_need_space(obuf, DECIMAL_STR_MAX(x));
    obuf->count += buf_uint_to_str(x, obuf->buf + obuf->count);
}

void term_hide_cursor(Terminal *term)
{
    term_add_strview(&term->obuf, term->control_codes.hide_cursor);
}

void term_show_cursor(Terminal *term)
{
    term_add_strview(&term->obuf, term->control_codes.show_cursor);
}

void term_move_cursor(TermOutputBuffer *obuf, unsigned int x, unsigned int y)
{
    term_add_literal(obuf, "\033[");
    term_add_uint(obuf, y + 1);
    if (x != 0) {
        term_add_byte(obuf, ';');
        term_add_uint(obuf, x + 1);
    }
    term_add_byte(obuf, 'H');
}

void term_save_title(Terminal *term)
{
    term_add_strview(&term->obuf, term->control_codes.save_title);
}

void term_restore_title(Terminal *term)
{
    term_add_strview(&term->obuf, term->control_codes.restore_title);
}

void term_clear_eol(Terminal *term)
{
    TermOutputBuffer *obuf = &term->obuf;
    const size_t end = obuf->scroll_x + obuf->width;
    if (obuf->x >= end) {
        return;
    }
    if (
        obuf->can_clear
        && (obuf->color.bg < 0 || term->features & TFLAG_BACK_COLOR_ERASE)
        && !(obuf->color.attr & ATTR_REVERSE)
    ) {
        term_add_literal(obuf, "\033[K");
        obuf->x = end;
    } else {
        term_set_bytes(term, ' ', end - obuf->x);
    }
}

void term_clear_screen(TermOutputBuffer *obuf)
{
    term_add_literal (
        obuf,
        "\033[0m" // Reset colors and attributes
        "\033[H"  // Move cursor to 1,1 (done only to mimic terminfo(5) "clear")
        "\033[2J" // Clear whole screen (regardless of cursor position)
    );
}

void term_output_flush(TermOutputBuffer *obuf)
{
    if (obuf->count) {
        (void)!xwrite(STDOUT_FILENO, obuf->buf, obuf->count);
        obuf->count = 0;
    }
}

static void skipped_too_much(TermOutputBuffer *obuf, CodePoint u)
{
    size_t n = obuf->x - obuf->scroll_x;
    char *buf = obuf->buf + obuf->count;
    obuf_need_space(obuf, 8);
    if (u == '\t' && obuf->tab != TAB_CONTROL) {
        memset(buf, (obuf->tab == TAB_SPECIAL) ? '-' : ' ', n);
        obuf->count += n;
    } else if (u < 0x20) {
        *buf = u | 0x40;
        obuf->count++;
    } else if (u == 0x7f) {
        *buf = '?';
        obuf->count++;
    } else if (u_is_unprintable(u)) {
        char tmp[4];
        size_t idx = 0;
        u_set_hex(tmp, &idx, u);
        memcpy(buf, tmp + 4 - n, n);
        obuf->count += n;
    } else {
        *buf = '>';
        obuf->count++;
    }
}

static void buf_skip(TermOutputBuffer *obuf, CodePoint u)
{
    if (u < 0x80) {
        if (!ascii_iscntrl(u)) {
            obuf->x++;
        } else if (u == '\t' && obuf->tab != TAB_CONTROL) {
            size_t tw = obuf->tab_width;
            obuf->x += (obuf->x + tw) / tw * tw - obuf->x;
        } else {
            // Control
            obuf->x += 2;
        }
    } else {
        // u_char_width() needed to handle 0x80-0x9f even if term_utf8 is false
        obuf->x += u_char_width(u);
    }

    if (obuf->x > obuf->scroll_x) {
        skipped_too_much(obuf, u);
    }
}

static void print_tab(TermOutputBuffer *obuf, size_t width)
{
    char ch = ' ';
    if (unlikely(obuf->tab == TAB_SPECIAL)) {
        obuf->buf[obuf->count++] = '>';
        obuf->x++;
        width--;
        ch = '-';
    }
    if (width > 0) {
        memset(obuf->buf + obuf->count, ch, width);
        obuf->count += width;
        obuf->x += width;
    }
}

bool term_put_char(TermOutputBuffer *obuf, CodePoint u)
{
    if (unlikely(obuf->x < obuf->scroll_x)) {
        // Scrolled, char (at least partially) invisible
        buf_skip(obuf, u);
        return true;
    }

    const size_t space = obuf->scroll_x + obuf->width - obuf->x;
    if (unlikely(!space)) {
        return false;
    }

    obuf_need_space(obuf, 8);
    if (likely(u < 0x80)) {
        if (likely(!ascii_iscntrl(u))) {
            obuf->buf[obuf->count++] = u;
            obuf->x++;
        } else if (u == '\t' && obuf->tab != TAB_CONTROL) {
            const size_t tw = obuf->tab_width;
            const size_t x = obuf->x;
            size_t width = (x + tw) / tw * tw - x;
            if (unlikely(width > space)) {
                width = space;
            }
            print_tab(obuf, width);
        } else {
            // Use caret notation for control chars:
            obuf->buf[obuf->count++] = '^';
            obuf->x++;
            if (likely(space > 1)) {
                obuf->buf[obuf->count++] = (u + 64) & 0x7F;
                obuf->x++;
            }
        }
    } else {
        const size_t width = u_char_width(u);
        if (likely(width <= space)) {
            obuf->x += width;
            u_set_char(obuf->buf, &obuf->count, u);
        } else if (u_is_unprintable(u)) {
            // <xx> would not fit.
            // There's enough space in the buffer so render all 4 characters
            // but increment position less.
            size_t idx = obuf->count;
            u_set_hex(obuf->buf, &idx, u);
            obuf->count += space;
            obuf->x += space;
        } else {
            obuf->buf[obuf->count++] = '>';
            obuf->x++;
        }
    }

    return true;
}
