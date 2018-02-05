#include "obuf.h"
#include "common.h"
#include "uchar.h"

OutputBuffer obuf;

static size_t obuf_avail(void)
{
    return sizeof(obuf.buf) - obuf.count;
}

static void obuf_need_space(size_t count)
{
    if (obuf_avail() < count) {
        buf_flush();
    }
}

void buf_reset(size_t start_x, size_t width, size_t scroll_x)
{
    obuf.x = 0;
    obuf.width = width;
    obuf.scroll_x = scroll_x;
    obuf.tab_width = 8;
    obuf.tab = TAB_CONTROL;
    obuf.can_clear = start_x + width == terminal.width;
}

// Does not update obuf.x
void buf_add_bytes(const char *const str, size_t count)
{
    if (count > obuf_avail()) {
        buf_flush();
        if (count >= sizeof(obuf.buf)) {
            xwrite(1, str, count);
            return;
        }
    }
    memcpy(obuf.buf + obuf.count, str, count);
    obuf.count += count;
}

void buf_set_bytes(char ch, size_t count)
{
    int skip;

    if (obuf.x + count > obuf.scroll_x + obuf.width) {
        count = obuf.scroll_x + obuf.width - obuf.x;
    }

    skip = obuf.scroll_x - obuf.x;
    if (skip > 0) {
        if (skip > count) {
            skip = count;
        }
        obuf.x += skip;
        count -= skip;
    }

    obuf.x += count;
    while (count) {
        size_t avail, n = count;

        obuf_need_space(1);
        avail = obuf_avail();
        if (n > avail) {
            n = avail;
        }

        memset(obuf.buf + obuf.count, ch, n);
        obuf.count += n;
        count -= n;
    }
}

// Does not update obuf.x
void buf_add_ch(char ch)
{
    obuf_need_space(1);
    obuf.buf[obuf.count++] = ch;
}

void buf_escape(const char *const str)
{
    buf_add_bytes(str, strlen(str));
}

void buf_add_str(const char *const str)
{
    size_t i = 0;
    while (str[i]) {
        if (!buf_put_char(u_str_get_char(str, &i))) {
            break;
        }
    }
}

void buf_hide_cursor(void)
{
    if (terminal.control_codes->hide_cursor) {
        buf_escape(terminal.control_codes->hide_cursor);
    }
}

void buf_show_cursor(void)
{
    if (terminal.control_codes->show_cursor) {
        buf_escape(terminal.control_codes->show_cursor);
    }
}

// Move cursor to screen position (x and y are zero-based)
void buf_move_cursor(int x, int y)
{
    if (x < 0 || x >= 999 || y < 0 || y >= 999) {
        return;
    }
    static char buf[16];
    const int len = snprintf (
        buf,
        11, // == strlen("\033[998;998H") + 1 (for NUL, to avoid truncation)
        "\033[%u;%uH",
        ((unsigned int)y) + 1,
        ((unsigned int)x) + 1
    );
    BUG_ON(len < 6);  //  6 == strlen("\033[0;0H")
    BUG_ON(len > 10); // 10 == strlen("\033[998;998H")
    buf_add_bytes(buf, (size_t)len);
}

void buf_set_color(const TermColor *const color)
{
    if (!memcmp(color, &obuf.color, sizeof(*color))) {
        return;
    }

    TermColor c = *color;

    // TERM=xterm: 8 colors
    // TERM=linux: 8 colors. colors > 7 corrupt screen
    if (terminal.max_colors < 16 && c.fg >= 8 && c.fg <= 15) {
        c.attr |= ATTR_BOLD;
        c.fg &= 7;
    }

    // Max 34 bytes (3 + 6 * 2 + 2 * 9 + 1)
    char buf[64];
    size_t i = 0;
    buf[i++] = '\033';
    buf[i++] = '[';
    buf[i++] = '0';

    if (c.attr & ATTR_BOLD) {
        buf[i++] = ';';
        buf[i++] = '1';
    }
    if (c.attr & ATTR_LOW_INTENSITY) {
        buf[i++] = ';';
        buf[i++] = '2';
    }
    if (c.attr & ATTR_ITALIC) {
        buf[i++] = ';';
        buf[i++] = '3';
    }
    if (c.attr & ATTR_UNDERLINE) {
        buf[i++] = ';';
        buf[i++] = '4';
    }
    if (c.attr & ATTR_BLINKING) {
        buf[i++] = ';';
        buf[i++] = '5';
    }
    if (c.attr & ATTR_REVERSE_VIDEO) {
        buf[i++] = ';';
        buf[i++] = '7';
    }
    if (c.attr & ATTR_INVISIBLE_TEXT) {
        buf[i++] = ';';
        buf[i++] = '8';
    }
    if (c.fg >= 0) {
        const unsigned char fg = (unsigned char) c.fg;
        if (fg < 8) {
            buf[i++] = ';';
            buf[i++] = '3';
            buf[i++] = '0' + fg;
        } else {
            i += snprintf(&buf[i], 10, ";38;5;%u", fg);
        }
    }
    if (c.bg >= 0) {
        const unsigned char bg = (unsigned char) c.bg;
        if (bg < 8) {
            buf[i++] = ';';
            buf[i++] = '4';
            buf[i++] = '0' + bg;
        } else {
            i += snprintf(&buf[i], 10, ";48;5;%u", bg);
        }
    }
    buf[i++] = 'm';

    buf_add_bytes(buf, i);
    obuf.color = *color;
}

void buf_clear_eol(void)
{
    if (obuf.x < obuf.scroll_x + obuf.width) {
        if (
            obuf.can_clear
            && terminal.control_codes->clear_to_eol
            && (obuf.color.bg < 0 || terminal.can_bg_color_erase)
        ) {
            buf_escape(terminal.control_codes->clear_to_eol);
            obuf.x = obuf.scroll_x + obuf.width;
        } else {
            buf_set_bytes(' ', obuf.scroll_x + obuf.width - obuf.x);
        }
    }
}

void buf_flush(void)
{
    if (obuf.count) {
        xwrite(1, obuf.buf, obuf.count);
        obuf.count = 0;
    }
}

static void skipped_too_much(CodePoint u)
{
    size_t n = obuf.x - obuf.scroll_x;

    obuf_need_space(8);
    if (u == '\t' && obuf.tab != TAB_CONTROL) {
        char ch = ' ';
        if (obuf.tab == TAB_SPECIAL) {
            ch = '-';
        }
        memset(obuf.buf + obuf.count, ch, n);
        obuf.count += n;
    } else if (u < 0x20) {
        obuf.buf[obuf.count++] = u | 0x40;
    } else if (u == 0x7f) {
        obuf.buf[obuf.count++] = '?';
    } else if (u_is_unprintable(u)) {
        char tmp[4];
        size_t idx = 0;
        u_set_hex(tmp, &idx, u);
        memcpy(obuf.buf + obuf.count, tmp + 4 - n, n);
        obuf.count += n;
    } else {
        obuf.buf[obuf.count++] = '>';
    }
}

static void buf_skip(CodePoint u)
{
    if (likely(u < 0x80)) {
        if (likely(!u_is_ctrl(u))) {
            obuf.x++;
        } else if (u == '\t' && obuf.tab != TAB_CONTROL) {
            obuf.x += (obuf.x + obuf.tab_width) / obuf.tab_width * obuf.tab_width - obuf.x;
        } else {
            // Control
            obuf.x += 2;
        }
    } else {
        // u_char_width() needed to handle 0x80-0x9f even if term_utf8 is false
        obuf.x += u_char_width(u);
    }

    if (obuf.x > obuf.scroll_x) {
        skipped_too_much(u);
    }
}

static void print_tab(size_t width)
{
    char ch = ' ';

    if (obuf.tab == TAB_SPECIAL) {
        obuf.buf[obuf.count++] = '>';
        obuf.x++;
        width--;
        ch = '-';
    }
    if (width > 0) {
        memset(obuf.buf + obuf.count, ch, width);
        obuf.count += width;
        obuf.x += width;
    }
}

bool buf_put_char(CodePoint u)
{
    size_t space = obuf.scroll_x + obuf.width - obuf.x;
    size_t width;

    if (obuf.x < obuf.scroll_x) {
        // Scrolled, char (at least partially) invisible
        buf_skip(u);
        return true;
    }

    if (!space) {
        return false;
    }

    obuf_need_space(8);
    if (likely(u < 0x80)) {
        if (likely(!u_is_ctrl(u))) {
            obuf.buf[obuf.count++] = u;
            obuf.x++;
        } else if (u == '\t' && obuf.tab != TAB_CONTROL) {
            width = (obuf.x + obuf.tab_width) / obuf.tab_width * obuf.tab_width - obuf.x;
            if (width > space) {
                width = space;
            }
            print_tab(width);
        } else {
            u_set_ctrl(obuf.buf, &obuf.count, u);
            obuf.x += 2;
            if (unlikely(space == 1)) {
                // Wrote too much
                obuf.count--;
                obuf.x--;
            }
        }
    } else {
        width = u_char_width(u);
        if (width <= space) {
            obuf.x += width;
            u_set_char(obuf.buf, &obuf.count, u);
        } else if (u_is_unprintable(u)) {
            // <xx> would not fit.
            // There's enough space in the buffer so render all 4 characters
            // but increment position less.
            size_t idx = obuf.count;
            u_set_hex(obuf.buf, &idx, u);
            obuf.count += space;
            obuf.x += space;
        } else {
            obuf.buf[obuf.count++] = '>';
            obuf.x++;
        }
    }
    return true;
}
