#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "output.h"
#include "terminal.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/utf8.h"
#include "util/xreadwrite.h"

TermOutputBuffer obuf;

static void obuf_need_space(size_t count)
{
    if (unlikely(obuf_avail() < count)) {
        term_output_flush();
    }
}

void term_output_reset(size_t start_x, size_t width, size_t scroll_x)
{
    obuf.x = 0;
    obuf.width = width;
    obuf.scroll_x = scroll_x;
    obuf.tab_width = 8;
    obuf.tab = TAB_CONTROL;
    obuf.can_clear = start_x + width == terminal.width;
}

// Does not update obuf.x
void term_add_bytes(const char *str, size_t count)
{
    if (unlikely(count > obuf_avail())) {
        term_output_flush();
        if (unlikely(count >= sizeof(obuf.buf))) {
            xwrite(STDOUT_FILENO, str, count);
            return;
        }
    }
    memcpy(obuf.buf + obuf.count, str, count);
    obuf.count += count;
}

void term_repeat_byte(char ch, size_t count)
{
    while (count) {
        obuf_need_space(1);
        size_t avail = obuf_avail();
        size_t n = (count > avail) ? avail : count;
        memset(obuf.buf + obuf.count, ch, n);
        obuf.count += n;
        count -= n;
    }
}

void term_set_bytes(char ch, size_t count)
{
    if (obuf.x + count > obuf.scroll_x + obuf.width) {
        count = obuf.scroll_x + obuf.width - obuf.x;
    }
    ssize_t skip = obuf.scroll_x - obuf.x;
    if (skip > 0) {
        if (skip > count) {
            skip = count;
        }
        obuf.x += skip;
        count -= skip;
    }
    obuf.x += count;
    terminal.repeat_byte(ch, count);
}

// Does not update obuf.x
void term_add_byte(char ch)
{
    obuf_need_space(1);
    obuf.buf[obuf.count++] = ch;
}

void term_add_string_view(StringView sv)
{
    if (sv.length) {
        term_add_bytes(sv.data, sv.length);
    }
}

void term_add_str(const char *str)
{
    size_t i = 0;
    while (str[i]) {
        if (!term_put_char(u_str_get_char(str, &i))) {
            break;
        }
    }
}

// Does not update obuf.x
void term_add_uint(unsigned int x)
{
    obuf_need_space(DECIMAL_STR_MAX(x));
    obuf.count += buf_uint_to_str(x, obuf.buf + obuf.count);
}

void term_hide_cursor(void)
{
    terminal.put_control_code(terminal.control_codes.hide_cursor);
}

void term_show_cursor(void)
{
    terminal.put_control_code(terminal.control_codes.show_cursor);
}

void term_move_cursor(unsigned int x, unsigned int y)
{
    terminal.move_cursor(x, y);
}

void term_save_title(void)
{
    terminal.put_control_code(terminal.control_codes.save_title);
}

void term_restore_title(void)
{
    terminal.put_control_code(terminal.control_codes.restore_title);
}

void term_clear_eol(void)
{
    const size_t end = obuf.scroll_x + obuf.width;
    if (obuf.x >= end) {
        return;
    }
    if (
        obuf.can_clear
        && (obuf.color.bg < 0 || terminal.back_color_erase)
        && !(obuf.color.attr & ATTR_REVERSE)
    ) {
        terminal.clear_to_eol();
        obuf.x = end;
    } else {
        term_set_bytes(' ', end - obuf.x);
    }
}

void term_output_flush(void)
{
    if (obuf.count) {
        xwrite(STDOUT_FILENO, obuf.buf, obuf.count);
        obuf.count = 0;
    }
}

static void skipped_too_much(CodePoint u)
{
    size_t n = obuf.x - obuf.scroll_x;
    char *buf = obuf.buf + obuf.count;
    obuf_need_space(8);
    if (u == '\t' && obuf.tab != TAB_CONTROL) {
        memset(buf, (obuf.tab == TAB_SPECIAL) ? '-' : ' ', n);
        obuf.count += n;
    } else if (u < 0x20) {
        *buf = u | 0x40;
        obuf.count++;
    } else if (u == 0x7f) {
        *buf = '?';
        obuf.count++;
    } else if (u_is_unprintable(u)) {
        char tmp[4];
        size_t idx = 0;
        u_set_hex(tmp, &idx, u);
        memcpy(buf, tmp + 4 - n, n);
        obuf.count += n;
    } else {
        *buf = '>';
        obuf.count++;
    }
}

static void buf_skip(CodePoint u)
{
    if (u < 0x80) {
        if (!ascii_iscntrl(u)) {
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
    if (unlikely(obuf.tab == TAB_SPECIAL)) {
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

bool term_put_char(CodePoint u)
{
    if (obuf.x < obuf.scroll_x) {
        // Scrolled, char (at least partially) invisible
        buf_skip(u);
        return true;
    }

    const size_t space = obuf.scroll_x + obuf.width - obuf.x;
    if (!space) {
        return false;
    }

    obuf_need_space(8);
    if (likely(u < 0x80)) {
        if (likely(!ascii_iscntrl(u))) {
            obuf.buf[obuf.count++] = u;
            obuf.x++;
        } else if (u == '\t' && obuf.tab != TAB_CONTROL) {
            const size_t tw = obuf.tab_width;
            const size_t x = obuf.x;
            size_t width = (x + tw) / tw * tw - x;
            if (width > space) {
                width = space;
            }
            print_tab(width);
        } else {
            // Use caret notation for control chars:
            obuf.buf[obuf.count++] = '^';
            obuf.x++;
            if (space > 1) {
                obuf.buf[obuf.count++] = (u + 64) & 0x7F;
                obuf.x++;
            }
        }
    } else {
        const size_t width = u_char_width(u);
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
