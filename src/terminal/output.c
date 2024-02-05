#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "output.h"
#include "color.h"
#include "cursor.h"
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

static char *obuf_need_space(TermOutputBuffer *obuf, size_t count)
{
    BUG_ON(count > TERM_OUTBUF_SIZE);
    if (unlikely(obuf_avail(obuf) < count)) {
        term_output_flush(obuf);
    }
    return obuf->buf + obuf->count;
}

void term_output_reset(Terminal *term, size_t start_x, size_t width, size_t scroll_x)
{
    TermOutputBuffer *obuf = &term->obuf;
    obuf->x = 0;
    obuf->width = width;
    obuf->scroll_x = scroll_x;
    obuf->tab_width = 8;
    obuf->tab_mode = TAB_CONTROL;
    obuf->can_clear = ((start_x + width) == term->width);
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

// NOTE: does not update `obuf.x`; see term_put_byte()
void term_put_bytes(TermOutputBuffer *obuf, const char *str, size_t count)
{
    if (unlikely(count >= TERM_OUTBUF_SIZE)) {
        term_output_flush(obuf);
        if (term_direct_write(str, count)) {
            LOG_INFO("writing %zu bytes directly to terminal", count);
        }
        return;
    }

    char *buf = obuf_need_space(obuf, count);
    obuf->count += count;
    memcpy(buf, str, count);
}

static void term_repeat_byte(TermOutputBuffer *obuf, char ch, size_t count)
{
    while (count) {
        char *buf = obuf_need_space(obuf, 1);
        size_t avail = obuf_avail(obuf);
        size_t n = MIN(count, avail);
        memset(buf, ch, n);
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

    const size_t maxlen = STRLEN("_E[30000b");
    char *buf = obuf_need_space(obuf, maxlen);
    size_t i = 0;
    buf[i++] = ch;
    buf[i++] = '\033';
    buf[i++] = '[';
    i += buf_uint_to_str(count - 1, buf + i);
    buf[i++] = 'b';
    BUG_ON(i > maxlen);
    obuf->count += i;
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

// Append a single byte to the buffer.
// NOTE: this does not update `obuf.x`, since it can be used to write
// bytes within escape sequences without advancing the cursor position.
void term_put_byte(TermOutputBuffer *obuf, char ch)
{
    char *buf = obuf_need_space(obuf, 1);
    buf[0] = ch;
    obuf->count++;
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
    const size_t maxlen = STRLEN("E[;H") + (2 * DECIMAL_STR_MAX(x));
    char *buf = obuf_need_space(obuf, maxlen);
    size_t i = copyliteral(buf, "\033[");
    i += buf_uint_to_str(y + 1, buf + i);

    if (x != 0) {
        buf[i++] = ';';
        i += buf_uint_to_str(x + 1, buf + i);
    }

    buf[i++] = 'H';
    BUG_ON(i > maxlen);
    obuf->count += i;
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

bool term_can_clear_eol_with_el_sequence(const Terminal *term)
{
    const TermOutputBuffer *obuf = &term->obuf;
    bool bce = !!(term->features & TFLAG_BACK_COLOR_ERASE);
    bool rev = !!(obuf->style.attr & ATTR_REVERSE);
    bool bg = (obuf->style.bg >= COLOR_BLACK);
    return obuf->can_clear && (bce || !bg) && !rev;
}

void term_clear_eol(Terminal *term)
{
    TermOutputBuffer *obuf = &term->obuf;
    const size_t end = obuf->scroll_x + obuf->width;
    if (obuf->x >= end) {
        return;
    }

    if (term_can_clear_eol_with_el_sequence(term)) {
        obuf->x = end;
        term_put_literal(obuf, "\033[K");
        return;
    }

    term_set_bytes(term, ' ', end - obuf->x);
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
    size_t n = obuf->count;
    if (n) {
        obuf->count = 0;
        term_direct_write(obuf->buf, n);
    }
}

static const char *get_tab_str(TermTabOutputMode tab_mode)
{
    static const char tabstr[][8] = {
        [TAB_NORMAL]  = "        ",
        [TAB_SPECIAL] = ">-------",
        // TAB_CONTROL is printed with u_set_char() and is thus omitted
    };
    BUG_ON(tab_mode >= ARRAYLEN(tabstr));
    return tabstr[tab_mode];
}

static void skipped_too_much(TermOutputBuffer *obuf, CodePoint u)
{
    char *buf = obuf_need_space(obuf, 7);
    size_t n = obuf->x - obuf->scroll_x;
    BUG_ON(n == 0);

    if (u == '\t' && obuf->tab_mode != TAB_CONTROL) {
        static_assert(TAB_WIDTH_MAX == 8);
        BUG_ON(n > 7);
        memcpy(buf, get_tab_str(obuf->tab_mode) + 1, 7);
        obuf->count += n;
        return;
    }

    if (u < 0x20 || u == 0x7F) {
        BUG_ON(n != 1);
        buf[0] = (u + 64) & 0x7F;
        obuf->count++;
        return;
    }

    if (u_is_unprintable(u)) {
        BUG_ON(n > 3);
        char tmp[8] = {'\0'};
        u_set_hex(tmp, u);
        memcpy(buf, tmp + 4 - n, 4);
        obuf->count += n;
        return;
    }

    BUG_ON(n != 1);
    buf[0] = '>';
    obuf->count++;
}

static void buf_skip(TermOutputBuffer *obuf, CodePoint u)
{
    if (u == '\t' && obuf->tab_mode != TAB_CONTROL) {
        obuf->x = next_indent_width(obuf->x, obuf->tab_width);
    } else {
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

    obuf_need_space(obuf, 8);
    if (likely(u < 0x80)) {
        if (likely(!ascii_iscntrl(u))) {
            obuf->buf[obuf->count++] = u;
            obuf->x++;
        } else if (u == '\t' && obuf->tab_mode != TAB_CONTROL) {
            size_t width = next_indent_width(obuf->x, obuf->tab_width) - obuf->x;
            BUG_ON(width > 8);
            width = MIN(width, space);
            memcpy(obuf->buf + obuf->count, get_tab_str(obuf->tab_mode), 8);
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
            obuf->count += u_set_char(obuf->buf + obuf->count, u);
        } else if (u_is_unprintable(u)) {
            // <xx> would not fit.
            // There's enough space in the buffer so render all 4 characters
            // but increment position less.
            u_set_hex(obuf->buf + obuf->count, u);
            obuf->count += space;
            obuf->x += space;
        } else {
            obuf->buf[obuf->count++] = '>';
            obuf->x++;
        }
    }

    return true;
}

static size_t set_color_suffix(char *buf, int32_t color)
{
    BUG_ON(color < 0);
    if (likely(color < 16)) {
        buf[0] = '0' + (color & 7);
        return 1;
    }

    if (!color_is_rgb(color)) {
        BUG_ON(color > 255);
        size_t i = copyliteral(buf, "8;5;");
        return i + buf_u8_to_str(color, buf + i);
    }

    uint8_t r, g, b;
    color_split_rgb(color, &r, &g, &b);
    size_t i = copyliteral(buf, "8;2;");
    i += buf_u8_to_str(r, buf + i);
    buf[i++] = ';';
    i += buf_u8_to_str(g, buf + i);
    buf[i++] = ';';
    i += buf_u8_to_str(b, buf + i);
    return i;
}

static size_t set_fg_color(char *buf, int32_t color)
{
    if (color < 0) {
        return 0;
    }

    bool light = (color >= 8 && color <= 15);
    buf[0] = ';';
    buf[1] = light ? '9' : '3';
    return 2 + set_color_suffix(buf + 2, color);
}

static size_t set_bg_color(char *buf, int32_t color)
{
    if (color < 0) {
        return 0;
    }

    bool light = (color >= 8 && color <= 15);
    buf[0] = ';';
    buf[1] = light ? '1' : '4';
    buf[2] = '0';
    size_t i = light ? 3 : 2;
    return i + set_color_suffix(buf + i, color);
}

static int32_t color_normalize(int32_t color)
{
    BUG_ON(!color_is_valid(color));
    return (color <= COLOR_KEEP) ? COLOR_DEFAULT : color;
}

static void term_style_sanitize(TermStyle *style, unsigned int ncv_attrs)
{
    // Replace COLOR_KEEP fg/bg colors with COLOR_DEFAULT, to normalize the
    // values set in TermOutputBuffer::style
    style->fg = color_normalize(style->fg);
    style->bg = color_normalize(style->bg);

    // Unset ATTR_KEEP, since it's meaningless at this stage (and shouldn't
    // be set in TermOutputBuffer::style)
    style->attr &= ~ATTR_KEEP;

    // Unset ncv_attrs bits, if fg and/or bg color is non-default (see "ncv"
    // in terminfo(5) man page)
    bool have_color = (style->fg > COLOR_DEFAULT || style->bg > COLOR_DEFAULT);
    style->attr &= (have_color ? ~ncv_attrs : ~0u);
}

void term_set_style(Terminal *term, TermStyle style)
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

    // TODO: take `TermOutputBuffer::style` into account and only emit
    // the minimal set of parameters needed to update the terminal's
    // current state (i.e. without using `0` to reset or emitting
    // already active attributes/colors)

    term_style_sanitize(&style, term->ncv_attributes);

    const size_t maxcolor = STRLEN(";38;2;255;255;255");
    const size_t maxlen = STRLEN("E[0m") + (2 * maxcolor) + (2 * ARRAYLEN(attr_map));
    char *buf = obuf_need_space(&term->obuf, maxlen);
    size_t pos = copyliteral(buf, "\033[0");

    for (size_t i = 0; i < ARRAYLEN(attr_map); i++) {
        if (style.attr & attr_map[i].attr) {
            buf[pos++] = ';';
            buf[pos++] = attr_map[i].code;
        }
    }

    pos += set_fg_color(buf + pos, style.fg);
    pos += set_bg_color(buf + pos, style.bg);
    buf[pos++] = 'm';
    BUG_ON(pos > maxlen);
    term->obuf.count += pos;
    term->obuf.style = style;
}

static void cursor_style_normalize(TermCursorStyle *s)
{
    BUG_ON(!cursor_type_is_valid(s->type));
    BUG_ON(!cursor_color_is_valid(s->color));
    s->type = (s->type == CURSOR_KEEP) ? CURSOR_DEFAULT : s->type;
    s->color = (s->color == COLOR_KEEP) ? COLOR_DEFAULT : s->color;
}

void term_set_cursor_style(Terminal *term, TermCursorStyle s)
{
    TermOutputBuffer *obuf = &term->obuf;
    cursor_style_normalize(&s);
    obuf->cursor_style = s;

    const size_t maxlen = STRLEN("E[7 qE]12;rgb:aa/bb/ccST");
    char *buf = obuf_need_space(obuf, maxlen);
    size_t i = 0;

    // Set shape with DECSCUSR
    BUG_ON(s.type < 0 || s.type > 9);
    buf[i++] = '\033';
    buf[i++] = '[';
    buf[i++] = '0' + s.type;
    buf[i++] = ' ';
    buf[i++] = 'q';

    if (s.color == COLOR_DEFAULT) {
        // Reset color with OSC 112
        i += copyliteral(buf + i, "\033]112");
    } else {
        // Set RGB color with OSC 12
        uint8_t r, g, b;
        color_split_rgb(s.color, &r, &g, &b);
        i += copyliteral(buf + i, "\033]12;rgb:");
        i += hex_encode_byte(buf + i, r);
        buf[i++] = '/';
        i += hex_encode_byte(buf + i, g);
        buf[i++] = '/';
        i += hex_encode_byte(buf + i, b);
    }

    buf[i++] = '\033';
    buf[i++] = '\\';
    BUG_ON(i > maxlen);
    obuf->count += i;
}
