#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "output.h"
#include "color.h"
#include "indent.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/log.h"
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
    BUG_ON(count > TERM_OUTBUF_SIZE);
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
    obuf->tab_mode = TAB_CONTROL;
    obuf->can_clear = start_x + width == term->width;
}

// Write directly to the terminal, as done when e.g. flushing the output buffer
static bool term_direct_write(const char *str, size_t count)
{
    ssize_t n = xwrite_all(STDOUT_FILENO, str, count);
    if (unlikely(n != count)) {
        LOG_ERRNO("write");
        return false;
    }
    return true;
}

// Does not update obuf.x
void term_put_bytes(TermOutputBuffer *obuf, const char *str, size_t count)
{
    if (unlikely(count > obuf_avail(obuf))) {
        term_output_flush(obuf);
        if (unlikely(count >= TERM_OUTBUF_SIZE)) {
            if (term_direct_write(str, count)) {
                LOG_INFO("writing %zu bytes directly to terminal", count);
            }
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
    term_put_byte(obuf, ch);
    term_put_literal(obuf, "\033[");
    term_put_uint(obuf, count - 1);
    term_put_byte(obuf, 'b');
}

void term_set_bytes(Terminal *term, char ch, size_t count)
{
    TermOutputBuffer *obuf = &term->obuf;
    if (obuf->x + count > obuf->scroll_x + obuf->width) {
        count = obuf->scroll_x + obuf->width - obuf->x;
    }
    ssize_t skip = obuf->scroll_x - obuf->x;
    if (skip > 0) {
        skip = MIN(skip, count);
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
void term_put_byte(TermOutputBuffer *obuf, char ch)
{
    obuf_need_space(obuf, 1);
    obuf->buf[obuf->count++] = ch;
}

void term_put_str(TermOutputBuffer *obuf, const char *str)
{
    size_t i = 0;
    while (str[i]) {
        if (!term_put_char(obuf, u_str_get_char(str, &i))) {
            break;
        }
    }
}

// Does not update obuf.x
void term_put_uint(TermOutputBuffer *obuf, unsigned int x)
{
    obuf_need_space(obuf, DECIMAL_STR_MAX(x));
    obuf->count += buf_uint_to_str(x, obuf->buf + obuf->count);
}

// Does not update obuf.x
static void term_put_u8_hex(TermOutputBuffer *obuf, uint8_t x)
{
    obuf_need_space(obuf, 2);
    hex_encode_byte(obuf->buf + obuf->count, x);
    obuf->count += 2;
}

// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-The-Alternate-Screen-Buffer
void term_use_alt_screen_buffer(Terminal *term)
{
    term_put_literal(&term->obuf, "\033[?1049h");
}

void term_use_normal_screen_buffer(Terminal *term)
{
    term_put_literal(&term->obuf, "\033[?1049l");
}

void term_hide_cursor(Terminal *term)
{
    term_put_literal(&term->obuf, "\033[?25l");
}

void term_show_cursor(Terminal *term)
{
    term_put_literal(&term->obuf, "\033[?25h");
}

void term_begin_sync_update(Terminal *term)
{
    if (term->features & TFLAG_SYNC_CSI) {
        term_put_literal(&term->obuf, "\033[?2026h");
    } else if (term->features & TFLAG_SYNC_DCS) {
        term_put_literal(&term->obuf, "\033P=1s\033\\");
    }
}

void term_end_sync_update(Terminal *term)
{
    if (term->features & TFLAG_SYNC_CSI) {
        term_put_literal(&term->obuf, "\033[?2026l");
    } else if (term->features & TFLAG_SYNC_DCS) {
        term_put_literal(&term->obuf, "\033P=2s\033\\");
    }
}

void term_move_cursor(TermOutputBuffer *obuf, unsigned int x, unsigned int y)
{
    term_put_literal(obuf, "\033[");
    term_put_uint(obuf, y + 1);
    if (x != 0) {
        term_put_byte(obuf, ';');
        term_put_uint(obuf, x + 1);
    }
    term_put_byte(obuf, 'H');
}

void term_save_title(Terminal *term)
{
    if (term->features & TFLAG_SET_WINDOW_TITLE) {
        term_put_literal(&term->obuf, "\033[22;2t");
    }
}

void term_restore_title(Terminal *term)
{
    if (term->features & TFLAG_SET_WINDOW_TITLE) {
        term_put_literal(&term->obuf, "\033[23;2t");
    }
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
        && (obuf->style.bg < 0 || term->features & TFLAG_BACK_COLOR_ERASE)
        && !(obuf->style.attr & ATTR_REVERSE)
    ) {
        term_put_literal(obuf, "\033[K");
        obuf->x = end;
    } else {
        term_set_bytes(term, ' ', end - obuf->x);
    }
}

void term_clear_screen(TermOutputBuffer *obuf)
{
    term_put_literal (
        obuf,
        "\033[0m" // Reset colors and attributes
        "\033[H"  // Move cursor to 1,1 (done only to mimic terminfo(5) "clear")
        "\033[2J" // Clear whole screen (regardless of cursor position)
    );
}

void term_output_flush(TermOutputBuffer *obuf)
{
    if (obuf->count) {
        term_direct_write(obuf->buf, obuf->count);
        obuf->count = 0;
    }
}

static void skipped_too_much(TermOutputBuffer *obuf, CodePoint u)
{
    size_t n = obuf->x - obuf->scroll_x;
    char *buf = obuf->buf + obuf->count;
    obuf_need_space(obuf, 8);
    if (u == '\t' && obuf->tab_mode != TAB_CONTROL) {
        memset(buf, (obuf->tab_mode == TAB_SPECIAL) ? '-' : ' ', n);
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
        } else if (u == '\t' && obuf->tab_mode != TAB_CONTROL) {
            obuf->x = next_indent_width(obuf->x, obuf->tab_width);
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

    static const char tabstr[][8] = {
        [TAB_NORMAL]  = "        ",
        [TAB_SPECIAL] = ">-------",
    };

    obuf_need_space(obuf, 8);
    if (likely(u < 0x80)) {
        if (likely(!ascii_iscntrl(u))) {
            obuf->buf[obuf->count++] = u;
            obuf->x++;
        } else if (u == '\t' && obuf->tab_mode != TAB_CONTROL) {
            size_t width = next_indent_width(obuf->x, obuf->tab_width) - obuf->x;
            BUG_ON(width > 8);
            BUG_ON(obuf->tab_mode >= ARRAYLEN(tabstr));
            width = MIN(width, space);
            memcpy(obuf->buf + obuf->count, tabstr[obuf->tab_mode], 8);
            obuf->count += width;
            obuf->x += width;
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

static void do_set_color(TermOutputBuffer *obuf, int32_t color, char ch)
{
    if (color < 0) {
        return;
    }

    term_put_byte(obuf, ';');
    term_put_byte(obuf, ch);

    if (likely(color < 8)) {
        term_put_byte(obuf, '0' + color);
    } else if (color < 256) {
        term_put_literal(obuf, "8;5;");
        term_put_uint(obuf, color);
    } else {
        uint8_t r, g, b;
        color_split_rgb(color, &r, &g, &b);
        term_put_literal(obuf, "8;2;");
        term_put_uint(obuf, r);
        term_put_byte(obuf, ';');
        term_put_uint(obuf, g);
        term_put_byte(obuf, ';');
        term_put_uint(obuf, b);
    }
}

static bool attr_is_set(const TermStyle *style, unsigned int attr, unsigned int ncv_attrs)
{
    if (style->attr & attr) {
        if (unlikely(ncv_attrs & attr)) {
            // Terminal only allows attr when not using colors
            return style->fg == COLOR_DEFAULT && style->bg == COLOR_DEFAULT;
        }
        return true;
    }
    return false;
}

void term_set_style(Terminal *term, const TermStyle *style)
{
    static const struct {
        char code;
        unsigned int attr;
    } attr_map[] = {
        {'1', ATTR_BOLD},
        {'2', ATTR_DIM},
        {'3', ATTR_ITALIC},
        {'4', ATTR_UNDERLINE},
        {'5', ATTR_BLINK},
        {'7', ATTR_REVERSE},
        {'8', ATTR_INVIS},
        {'9', ATTR_STRIKETHROUGH}
    };

    TermOutputBuffer *obuf = &term->obuf;
    term_put_literal(obuf, "\033[0");

    unsigned int ncv_attributes = term->ncv_attributes;
    for (size_t i = 0; i < ARRAYLEN(attr_map); i++) {
        if (attr_is_set(style, attr_map[i].attr, ncv_attributes)) {
            term_put_byte(obuf, ';');
            term_put_byte(obuf, attr_map[i].code);
        }
    }

    do_set_color(obuf, style->fg, '3');
    do_set_color(obuf, style->bg, '4');
    term_put_byte(obuf, 'm');
    obuf->style = *style;
}

void term_set_cursor_style(Terminal *term, TermCursorStyle s)
{
    TermCursorType type = (s.type == CURSOR_KEEP) ? CURSOR_DEFAULT : s.type;
    BUG_ON(type < 0 || type > 6);
    BUG_ON(s.color <= COLOR_INVALID);

    // Set shape with DECSCUSR
    TermOutputBuffer *obuf = &term->obuf;
    term_put_literal(obuf, "\033[");
    term_put_uint(obuf, type);
    term_put_literal(obuf, " q");

    if (s.color == COLOR_DEFAULT || s.color == COLOR_KEEP) {
        // Reset color with OSC 112
        term_put_literal(obuf, "\033]112\033\\");
    } else {
        // Set RGB color with OSC 12
        uint8_t r, g, b;
        color_split_rgb(s.color, &r, &g, &b);
        term_put_literal(obuf, "\033]12;rgb:");
        term_put_u8_hex(obuf, r);
        term_put_byte(obuf, '/');
        term_put_u8_hex(obuf, g);
        term_put_byte(obuf, '/');
        term_put_u8_hex(obuf, b);
        term_put_literal(obuf, "\033\\");
    }

    obuf->cursor_style = s;
}
