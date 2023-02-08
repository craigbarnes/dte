#include "screen.h"
#include "indent.h"
#include "selection.h"
#include "syntax/highlight.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/utf8.h"

typedef struct {
    const View *view;
    size_t line_nr;
    size_t offset;
    ssize_t sel_so;
    ssize_t sel_eo;

    const unsigned char *line;
    size_t size;
    size_t pos;
    size_t indent_size;
    size_t trailing_ws_offset;
    const TermColor **colors;
} LineInfo;

// Like mask_color() but can change bg color only if it has not been changed yet
static void mask_color2(const ColorScheme *colors, TermColor *color, const TermColor *over)
{
    int32_t default_bg = colors->builtin[BC_DEFAULT].bg;
    if (over->bg != COLOR_KEEP && (color->bg == default_bg || color->bg < 0)) {
        color->bg = over->bg;
    }

    if (over->fg != COLOR_KEEP) {
        color->fg = over->fg;
    }

    if (!(over->attr & ATTR_KEEP)) {
        color->attr = over->attr;
    }
}

static void mask_selection_and_current_line (
    const ColorScheme *colors,
    const LineInfo *info,
    TermColor *color
) {
    if (info->offset >= info->sel_so && info->offset < info->sel_eo) {
        mask_color(color, &colors->builtin[BC_SELECTION]);
    } else if (info->line_nr == info->view->cy) {
        mask_color2(colors, color, &colors->builtin[BC_CURRENTLINE]);
    }
}

static bool is_non_text(CodePoint u, bool display_special)
{
    if (u == '\t') {
        return display_special;
    }
    return u < 0x20 || u == 0x7F || u_is_unprintable(u);
}

static WhitespaceErrorFlags get_ws_error(const LocalOptions *opts)
{
    WhitespaceErrorFlags taberrs = WSE_TAB_INDENT | WSE_TAB_AFTER_INDENT;
    WhitespaceErrorFlags extra = opts->expand_tab ? taberrs : WSE_SPACE_INDENT;
    return opts->ws_error | ((opts->ws_error & WSE_AUTO_INDENT) ? extra : 0);
}

static bool whitespace_error(const LineInfo *info, CodePoint u, size_t i)
{
    const View *view = info->view;
    WhitespaceErrorFlags flags = get_ws_error(&view->buffer->options);
    WhitespaceErrorFlags trailing = flags & (WSE_TRAILING | WSE_ALL_TRAILING);
    if (i >= info->trailing_ws_offset && trailing) {
        // Trailing whitespace
        if (
            // Cursor is not on this line
            info->line_nr != view->cy
            // or is positioned before any trailing whitespace
            || view->cx < info->trailing_ws_offset
            // or user explicitly wants trailing space under cursor highlighted
            || flags & WSE_ALL_TRAILING
        ) {
            return true;
        }
    }

    bool in_indent = (i < info->indent_size);
    if (u == '\t') {
        WhitespaceErrorFlags mask = in_indent ? WSE_TAB_INDENT : WSE_TAB_AFTER_INDENT;
        return (flags & mask) != 0;
    }
    if (!in_indent) {
        // All checks below here only apply to indentation
        return false;
    }

    const char *line = info->line;
    size_t pos = i;
    size_t count = 0;
    while (pos > 0 && line[pos - 1] == ' ') {
        pos--;
    }
    while (pos < info->size && line[pos] == ' ') {
        pos++;
        count++;
    }

    WhitespaceErrorFlags mask;
    if (count >= view->buffer->options.tab_width) {
        // Spaces used instead of tab
        mask = WSE_SPACE_INDENT;
    } else if (pos < info->size && line[pos] == '\t') {
        // Space before tab
        mask = WSE_SPACE_INDENT;
    } else {
        // Less than tab width spaces at end of indentation
        mask = WSE_SPACE_ALIGN;
    }
    return (flags & mask) != 0;
}

static CodePoint screen_next_char(EditorState *e, LineInfo *info)
{
    size_t count, pos = info->pos;
    CodePoint u = info->line[pos];
    TermColor color;
    bool ws_error = false;

    if (likely(u < 0x80)) {
        info->pos++;
        count = 1;
        if (u == '\t' || u == ' ') {
            ws_error = whitespace_error(info, u, pos);
        }
    } else {
        u = u_get_nonascii(info->line, info->size, &info->pos);
        count = info->pos - pos;

        if (
            u_is_special_whitespace(u) // Highly annoying no-break space etc.
            && (info->view->buffer->options.ws_error & WSE_SPECIAL)
        ) {
            ws_error = true;
        }
    }

    if (info->colors && info->colors[pos]) {
        color = *info->colors[pos];
    } else {
        color = e->colors.builtin[BC_DEFAULT];
    }
    if (is_non_text(u, e->options.display_special)) {
        mask_color(&color, &e->colors.builtin[BC_NONTEXT]);
    }
    if (ws_error) {
        mask_color(&color, &e->colors.builtin[BC_WSERROR]);
    }
    mask_selection_and_current_line(&e->colors, info, &color);
    set_color(&e->terminal, &e->colors, &color);

    info->offset += count;
    return u;
}

static void screen_skip_char(TermOutputBuffer *obuf, LineInfo *info)
{
    CodePoint u = info->line[info->pos++];
    info->offset++;
    if (likely(u < 0x80)) {
        if (likely(!ascii_iscntrl(u))) {
            obuf->x++;
        } else if (u == '\t' && obuf->tab_mode != TAB_CONTROL) {
            obuf->x = next_indent_width(obuf->x, obuf->tab_width);
        } else {
            // Control
            obuf->x += 2;
        }
    } else {
        size_t pos = info->pos;
        info->pos--;
        u = u_get_nonascii(info->line, info->size, &info->pos);
        obuf->x += u_char_width(u);
        info->offset += info->pos - pos;
    }
}

static bool is_notice(const char *word, size_t len)
{
    switch (len) {
    case 3: return mem_equal(word, "XXX", 3);
    case 4: return mem_equal(word, "TODO", 4);
    case 5: return mem_equal(word, "FIXME", 5);
    }
    return false;
}

// Highlight certain words inside comments
static void hl_words(Terminal *term, const ColorScheme *colors, const LineInfo *info)
{
    const TermColor *cc = find_color(colors, "comment");
    const TermColor *nc = find_color(colors, "notice");

    if (!info->colors || !cc || !nc) {
        return;
    }

    size_t i = info->pos;
    if (i >= info->size) {
        return;
    }

    // Go to beginning of partially visible word inside comment
    while (i > 0 && info->colors[i] == cc && is_word_byte(info->line[i])) {
        i--;
    }

    // This should be more than enough. I'm too lazy to iterate characters
    // instead of bytes and calculate text width.
    const size_t max = info->pos + term->width * 4 + 8;

    size_t si;
    while (i < info->size) {
        if (info->colors[i] != cc || !is_word_byte(info->line[i])) {
            if (i > max) {
                break;
            }
            i++;
        } else {
            // Beginning of a word inside comment
            si = i++;
            while (
                i < info->size && info->colors[i] == cc
                && is_word_byte(info->line[i])
            ) {
                i++;
            }
            if (is_notice(info->line + si, i - si)) {
                for (size_t j = si; j < i; j++) {
                    info->colors[j] = nc;
                }
            }
        }
    }
}

static void line_info_init (
    LineInfo *info,
    const View *view,
    const BlockIter *bi,
    size_t line_nr
) {
    *info = (LineInfo) {
        .view = view,
        .line_nr = line_nr,
        .offset = block_iter_get_offset(bi),
    };

    if (!view->selection) {
        info->sel_so = -1;
        info->sel_eo = -1;
    } else if (view->sel_eo != SEL_EO_RECALC) {
        // Already calculated
        info->sel_so = view->sel_so;
        info->sel_eo = view->sel_eo;
        BUG_ON(info->sel_so > info->sel_eo);
    } else {
        SelectionInfo sel;
        init_selection(view, &sel);
        info->sel_so = sel.so;
        info->sel_eo = sel.eo;
    }
}

static void line_info_set_line (
    LineInfo *info,
    const StringView *line,
    const TermColor **colors
) {
    BUG_ON(line->length == 0);
    BUG_ON(line->data[line->length - 1] != '\n');

    info->line = line->data;
    info->size = line->length - 1;
    info->pos = 0;
    info->colors = colors;

    {
        size_t i, n;
        for (i = 0, n = info->size; i < n; i++) {
            char ch = info->line[i];
            if (ch != '\t' && ch != ' ') {
                break;
            }
        }
        info->indent_size = i;
    }

    static_assert_compatible_types(info->trailing_ws_offset, size_t);
    info->trailing_ws_offset = SIZE_MAX;
    for (ssize_t i = info->size - 1; i >= 0; i--) {
        char ch = info->line[i];
        if (ch != '\t' && ch != ' ') {
            break;
        }
        info->trailing_ws_offset = i;
    }
}

static void print_line(EditorState *e, LineInfo *info)
{
    // Screen might be scrolled horizontally. Skip most invisible
    // characters using screen_skip_char(), which is much faster than
    // buf_skip(screen_next_char(info)).
    //
    // There can be a wide character (tab, control code etc.) that is
    // partially visible and can't be skipped using screen_skip_char().
    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;
    while (obuf->x + 8 < obuf->scroll_x && info->pos < info->size) {
        screen_skip_char(obuf, info);
    }

    const ColorScheme *colors = &e->colors;
    hl_words(term, colors, info);

    while (info->pos < info->size) {
        BUG_ON(obuf->x > obuf->scroll_x + obuf->width);
        CodePoint u = screen_next_char(e, info);
        if (!term_put_char(obuf, u)) {
            // +1 for newline
            info->offset += info->size - info->pos + 1;
            return;
        }
    }

    TermColor color;
    if (e->options.display_special && obuf->x >= obuf->scroll_x) {
        // Syntax highlighter highlights \n but use default color anyway
        color = colors->builtin[BC_DEFAULT];
        mask_color(&color, &colors->builtin[BC_NONTEXT]);
        mask_selection_and_current_line(colors, info, &color);
        set_color(term, colors, &color);
        term_put_char(obuf, '$');
    }

    color = colors->builtin[BC_DEFAULT];
    mask_selection_and_current_line(colors, info, &color);
    set_color(term, colors, &color);
    info->offset++;
    term_clear_eol(term);
}

void update_range(EditorState *e, const View *view, long y1, long y2)
{
    const int edit_x = view->window->edit_x;
    const int edit_y = view->window->edit_y;
    const int edit_w = view->window->edit_w;
    const int edit_h = view->window->edit_h;

    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;
    term_output_reset(term, edit_x, edit_w, view->vx);
    obuf->tab_width = view->buffer->options.tab_width;
    obuf->tab_mode = e->options.display_special ? TAB_SPECIAL : TAB_NORMAL;

    BlockIter bi = view->cursor;
    for (long i = 0, n = view->cy - y1; i < n; i++) {
        block_iter_prev_line(&bi);
    }
    for (long i = 0, n = y1 - view->cy; i < n; i++) {
        block_iter_eat_line(&bi);
    }
    block_iter_bol(&bi);

    LineInfo info;
    line_info_init(&info, view, &bi, y1);

    y1 -= view->vy;
    y2 -= view->vy;

    bool got_line = !block_iter_is_eof(&bi);
    hl_fill_start_states(view->buffer, &e->colors, info.line_nr);
    long i;
    for (i = y1; got_line && i < y2; i++) {
        obuf->x = 0;
        term_move_cursor(obuf, edit_x, edit_y + i);

        StringView line;
        fill_line_nl_ref(&bi, &line);
        bool next_changed;
        const TermColor **colors = hl_line(view->buffer, &e->colors, &line, info.line_nr, &next_changed);
        line_info_set_line(&info, &line, colors);
        print_line(e, &info);

        got_line = !!block_iter_next_line(&bi);
        info.line_nr++;

        if (next_changed && i + 1 == y2 && y2 < edit_h) {
            // More lines need to be updated not because their contents have
            // changed but because their highlight state has
            y2++;
        }
    }

    if (i < y2 && info.line_nr == view->cy) {
        // Dummy empty line is shown only if cursor is on it
        TermColor color = e->colors.builtin[BC_DEFAULT];

        obuf->x = 0;
        mask_color2(&e->colors, &color, &e->colors.builtin[BC_CURRENTLINE]);
        set_color(term, &e->colors, &color);

        term_move_cursor(obuf, edit_x, edit_y + i++);
        term_clear_eol(term);
    }

    if (i < y2) {
        set_builtin_color(term, &e->colors, BC_NOLINE);
    }
    for (; i < y2; i++) {
        obuf->x = 0;
        term_move_cursor(obuf, edit_x, edit_y + i);
        term_put_char(obuf, '~');
        term_clear_eol(term);
    }
}
