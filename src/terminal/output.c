// Terminal output and control functions.
// Copyright © Craig Barnes.
// Copyright © Timo Hirvonen.
// SPDX-License-Identifier: GPL-2.0-only
// See also:
// • ECMA-48 5th edition, §8.3 (CUP, ED, EL, REP, SGR):
//   https://ecma-international.org/publications-and-standards/standards/ecma-48/
// • DEC Manual EK-VT510-RM, Chapter 5 (DECRQM, DECRQSS, DECSCUSR, DECTCEM):
//   https://vt100.net/docs/vt510-rm/contents.html
// • XTerm's ctlseqs.html (XTWINOPS, XTQMODKEYS, XTGETTCAP, OSC 12, OSC 112):
//   https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
// • Kitty's keyboard protocol documentation (CSI ? u):
//   https://sw.kovidgoyal.net/kitty/keyboard-protocol/

#include <limits.h>
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
#include "util/str-util.h"
#include "util/utf8.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

char *term_output_reserve_space(TermOutputBuffer *obuf, size_t count)
{
    BUG_ON(count > TERM_OUTBUF_SIZE);
    BUG_ON(obuf->count > TERM_OUTBUF_SIZE);
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

    char *buf = term_output_reserve_space(obuf, count);
    obuf->count += count;
    memcpy(buf, str, count);
}

static void term_repeat_byte(TermOutputBuffer *obuf, char ch, size_t count)
{
    if (likely(count <= TERM_OUTBUF_SIZE)) {
        // Repeat count fits in buffer; reserve space and tail-call memset(3)
        char *buf = term_output_reserve_space(obuf, count);
        obuf->count += count;
        memset(buf, ch, count);
        return;
    }

    // Repeat count greater than buffer size; fill buffer with `ch` and call
    // write() repeatedly until `count` reaches zero
    term_output_flush(obuf);
    memset(obuf->buf, ch, TERM_OUTBUF_SIZE);
    while (count) {
        size_t n = MIN(count, TERM_OUTBUF_SIZE);
        count -= n;
        term_direct_write(obuf->buf, n);
    }
}

static bool ecma48_repeat_byte(TermOutputBuffer *obuf, char ch, size_t count)
{
    if (!ascii_isprint(ch) || count < ECMA48_REP_MIN || count > ECMA48_REP_MAX) {
        term_repeat_byte(obuf, ch, count);
        return false;
    }

    // ECMA-48 REP (CSI Pn b)
    static_assert(ECMA48_REP_MAX == 30000);
    const size_t maxlen = STRLEN("_E[30000b");
    char *buf = term_output_reserve_space(obuf, maxlen);
    size_t i = 0;
    buf[i++] = ch;
    buf[i++] = '\033';
    buf[i++] = '[';
    i += buf_uint_to_str(count - 1, buf + i);
    buf[i++] = 'b';
    BUG_ON(i > maxlen);
    obuf->count += i;
    return true;
}

TermSetBytesMethod term_set_bytes(Terminal *term, char ch, size_t count)
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
        bool used_rep = ecma48_repeat_byte(obuf, ch, count);
        return used_rep ? TERM_SET_BYTES_REP : TERM_SET_BYTES_MEMSET;
    }

    term_repeat_byte(obuf, ch, count);
    return TERM_SET_BYTES_MEMSET;
}

// Append a single byte to the buffer.
// NOTE: this does not update `obuf.x`, since it can be used to write
// bytes within escape sequences without advancing the cursor position.
void term_put_byte(TermOutputBuffer *obuf, char ch)
{
    char *buf = term_output_reserve_space(obuf, 1);
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

/*
 * See also:
 * • handle_query_reply()
 * • parse_csi_query_reply()
 * • https://vt100.net/docs/vt510-rm/DA1.html
 * • ECMA-48 §8.3.24
 */
void term_put_initial_queries(Terminal *term, unsigned int level)
{
    if (level < 1) {
        return;
    }

    LOG_INFO("sending level 1 queries to terminal");
    term_put_literal(&term->obuf, "\033[c"); // ECMA-48 DA (AKA "DA1")

    if (level < 2) {
        return;
    }

    // Level 6 or greater means emit all query levels and also emit even
    // conditional queries, i.e. those that are usually omitted when the
    // corresponding feature flag was already set by term_init()
    bool emit_all = (level >= 6);
    if (emit_all) {
        LOG_INFO("query level set to %u; unconditionally sending all queries", level);
    }

    term_put_level_2_queries(term, emit_all);
    term->features |= TFLAG_QUERY_L2;

    if (level < 3) {
        return;
    }

    term_put_level_3_queries(term, emit_all);
    term->features |= TFLAG_QUERY_L3;
}

/*
 * See also:
 * • handle_query_reply()
 * • TFLAG_QUERY_L2
 * • parse_csi_query_reply()
 * • parse_dcs_query_reply()
 * • parse_xtwinops_query_reply()
 */
void term_put_level_2_queries(Terminal *term, bool emit_all)
{
    static const char queries[] =
        "\033[>0q" // XTVERSION (terminal name and version)
        "\033[>c" // DA2 (Secondary Device Attributes)
        "\033[?u" // Kitty keyboard protocol flags
        "\033[?1036$p" // DECRQM 1036 (metaSendsEscape; must be after kitty query)
        "\033[?1039$p" // DECRQM 1039 (altSendsEscape; must be after kitty query)
    ;

    static const char debug_queries[] =
        "\033[?4m" // XTQMODKEYS 4 (xterm modifyOtherKeys mode)
        "\033[?7$p" // DECRQM 7 (DECAWM; auto-wrap mode)
        "\033[?25$p" // DECRQM 25 (DECTCEM; cursor visibility)
        "\033[?45$p" // DECRQM 45 (XTREVWRAP; reverse-wraparound mode)
        "\033[?67$p" // DECRQM 67 (DECBKM; backspace key sends BS)
        "\033[?1049$p" // DECRQM 1049 (alternate screen buffer)
        "\033[?2004$p" // DECRQM 2004 (bracketed paste)
        "\033[18t" // XTWINOPS 18 (text area size in "characters"/cells)
    ;

    TermOutputBuffer *obuf = &term->obuf;
    LOG_INFO("sending level 2 queries to terminal");
    term_put_bytes(obuf, queries, sizeof(queries) - 1);

    TermFeatureFlags features = emit_all ? 0 : term->features;
    if (!(features & TFLAG_SYNC)) {
        term_put_literal(obuf, "\033[?2026$p"); // DECRQM 2026
    }

    // Debug query responses are used purely for logging/informational purposes
    if (emit_all || log_level_debug_enabled()) {
        term_put_bytes(obuf, debug_queries, sizeof(debug_queries) - 1);
    }
}

/*
 * Some terminals fail to parse DCS sequences in accordance with ECMA-48,
 * so DCS queries are sent separately and only after probing for some
 * known problem cases (e.g. PuTTY).
 *
 * See also:
 * • handle_query_reply()
 * • TFLAG_QUERY_L3
 * • parse_dcs_query_reply()
 * • parse_xtgettcap_reply()
 * • handle_decrqss_sgr_reply()
 */
void term_put_level_3_queries(Terminal *term, bool emit_all)
{
    // Note: the correct (according to ISO 8613-6) format for the SGR
    // sequence here would be "\033[0;38:2::60:70:80;48:5:255m", but we
    // use the standards-incorrect (but de facto more widely supported)
    // format because that's what is actually used in term_set_style()
    static const char sgr_query[] =
        "\033[0;38;2;60;70;80;48;5;255m" // SGR with direct fg and indexed bg
        "\033P$qm\033\\" // DECRQSS SGR (check support for SGR params above)
        "\033[0m" // SGR 0
    ;

    TermOutputBuffer *obuf = &term->obuf;
    TermFeatureFlags features = emit_all ? 0 : term->features;
    LOG_INFO("sending level 3 queries to terminal");

    if (!(features & TFLAG_TRUE_COLOR)) {
        term_put_bytes(obuf, sgr_query, sizeof(sgr_query) - 1);
    }

    if (!(features & TFLAG_BACK_COLOR_ERASE)) {
        term_put_literal(obuf, "\033P+q626365\033\\"); // XTGETTCAP "bce"
    }
    if (!(features & TFLAG_ECMA48_REPEAT)) {
        term_put_literal(obuf, "\033P+q726570\033\\"); // XTGETTCAP "rep"
    }
    if (!(features & TFLAG_SET_WINDOW_TITLE)) {
        term_put_literal(obuf, "\033P+q74736C\033\\"); // XTGETTCAP "tsl"
    }
    if (!(features & TFLAG_OSC52_COPY)) {
        term_put_literal(obuf, "\033P+q4D73\033\\"); // XTGETTCAP "Ms"
    }

    if (emit_all || log_level_debug_enabled()) {
        term_put_literal(obuf, "\033P$q q\033\\"); // DECRQSS DECSCUSR (cursor style)
    }
}

// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h2-The-Alternate-Screen-Buffer
void term_use_alt_screen_buffer(Terminal *term)
{
    term_put_literal(&term->obuf, "\033[?1049h"); // DECSET 1049
}

void term_use_normal_screen_buffer(Terminal *term)
{
    term_put_literal(&term->obuf, "\033[?1049l"); // DECRST 1049
}

void term_hide_cursor(Terminal *term)
{
    term_put_literal(&term->obuf, "\033[?25l"); // DECRST 25 (DECTCEM)
}

void term_show_cursor(Terminal *term)
{
    term_put_literal(&term->obuf, "\033[?25h"); // DECSET 25 (DECTCEM)
}

void term_begin_sync_update(Terminal *term)
{
    TermOutputBuffer *obuf = &term->obuf;
    WARN_ON(obuf->sync_pending);
    if (term->features & TFLAG_SYNC) {
        term_put_literal(obuf, "\033[?2026h"); // DECSET 2026
        obuf->sync_pending = true;
    }
}

void term_end_sync_update(Terminal *term)
{
    TermOutputBuffer *obuf = &term->obuf;
    if ((term->features & TFLAG_SYNC) && obuf->sync_pending) {
        term_put_literal(obuf, "\033[?2026l"); // DECRST 2026
        obuf->sync_pending = false;
    }
}

void term_move_cursor(TermOutputBuffer *obuf, unsigned int x, unsigned int y)
{
    // ECMA-48 CUP (CSI Pl ; Pc H)
    const size_t maxlen = STRLEN("E[;H") + (2 * DECIMAL_STR_MAX(x));
    char *buf = term_output_reserve_space(obuf, maxlen);
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
        // Save window title on stack (XTWINOPS)
        // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
        term_put_literal(&term->obuf, "\033[22;2t");
    }
}

void term_restore_title(Terminal *term)
{
    if (term->features & TFLAG_SET_WINDOW_TITLE) {
        // Restore window title from stack (XTWINOPS)
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

int term_clear_eol(Terminal *term)
{
    TermOutputBuffer *obuf = &term->obuf;
    const size_t end = obuf->scroll_x + obuf->width;
    if (obuf->x >= end) {
        // Cursor already at EOL; nothing to clear
        return 0;
    }

    if (term_can_clear_eol_with_el_sequence(term)) {
        obuf->x = end;
        term_put_literal(obuf, "\033[K"); // Erase to end of line (EL 0)
        static_assert(ECMA48_REP_MIN > -TERM_CLEAR_EOL_USED_EL);
        return TERM_CLEAR_EOL_USED_EL;
    }

    size_t count = end - obuf->x;
    TermSetBytesMethod method = term_set_bytes(term, ' ', count);

    if (unlikely(count > INT_MAX)) {
        // This is basically impossible, given that POSIX requires INT_MAX
        // to be at least 2³¹-1 and lines of that length simply aren't
        // something that happens. In any case, the return value here is
        // only ever used for logging purposes in update_status_line().
        LOG_ERROR("repeat count in %s() too large for int return value", __func__);
        return (method == TERM_SET_BYTES_REP) ? INT_MIN : INT_MAX;
    }

    // Return a negative count if an ECMA-48 REP sequence was used, or a
    // positive count if space was emitted `count` times
    return (method == TERM_SET_BYTES_REP) ? -count : count;
}

void term_clear_screen(TermOutputBuffer *obuf)
{
    term_put_literal (
        obuf,
        "\033[0m" // Reset colors and attributes (SGR 0)
        "\033[H"  // Move cursor to 1,1 (CUP; done only to mimic terminfo(5) "clear")
        "\033[2J" // Clear whole screen (ED 2)
    );
}

void term_output_flush(TermOutputBuffer *obuf)
{
    size_t n = obuf->count;
    if (n) {
        BUG_ON(n > TERM_OUTBUF_SIZE);
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
    char *buf = term_output_reserve_space(obuf, 7);
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
        static_assert(U_SET_HEX_LEN == 4);
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

    term_output_reserve_space(obuf, 8);
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

    size_t i = copyliteral(buf, "8;2;");
    i += buf_u8_to_str(color_r(color), buf + i);
    buf[i++] = ';';
    i += buf_u8_to_str(color_g(color), buf + i);
    buf[i++] = ';';
    i += buf_u8_to_str(color_b(color), buf + i);
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
    char *buf = term_output_reserve_space(&term->obuf, maxlen);
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
    char *buf = term_output_reserve_space(obuf, maxlen);

    // Set shape with DECSCUSR
    BUG_ON(s.type < 0 || s.type > 9);
    const char seq[] = {'\033', '[', '0' + s.type, ' ', 'q'};
    size_t i = copystrn(buf, seq, sizeof(seq));

    if (s.color == COLOR_DEFAULT) {
        // Reset color with OSC 112
        i += copyliteral(buf + i, "\033]112");
    } else {
        // Set RGB color with OSC 12
        i += copyliteral(buf + i, "\033]12;rgb:");
        i += hex_encode_byte(buf + i, color_r(s.color));
        buf[i++] = '/';
        i += hex_encode_byte(buf + i, color_g(s.color));
        buf[i++] = '/';
        i += hex_encode_byte(buf + i, color_b(s.color));
    }

    i += copyliteral(buf + i, "\033\\"); // String Terminator (ST)
    BUG_ON(i > maxlen);
    obuf->count += i;
}
