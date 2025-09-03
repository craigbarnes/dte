#include "ui.h"
#include "indent.h"
#include "selection.h"
#include "syntax/highlight.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/utf8.h"
#include "util/xstring.h"

typedef struct {
    const View *view;
    size_t line_nr;
    size_t offset;
    ssize_t sel_so;
    ssize_t sel_eo;
    WhitespaceErrorFlags wse_flags;

    const char *line;
    size_t size;
    size_t pos;
    size_t indent_size;
    size_t trailing_ws_offset;
    const TermStyle **styles;
} LineInfo;

static int32_t mask_color(int32_t color, int32_t over)
{
    return (over == COLOR_KEEP) ? color : over;
}

static unsigned int mask_attr(unsigned int attr, unsigned int over)
{
    return (over & ATTR_KEEP) ? attr : over;
}

static void mask_style(TermStyle *style, const TermStyle *over)
{
    *style = (TermStyle) {
        .fg = mask_color(style->fg, over->fg),
        .bg = mask_color(style->bg, over->bg),
        .attr = mask_attr(style->attr, over->attr),
    };
}

// Like mask_style() but can change bg color only if it has not been changed yet
static void mask_style2(TermStyle *style, const TermStyle *over, int32_t default_bg)
{
    bool has_default_bg = (style->bg == default_bg || style->bg <= COLOR_DEFAULT);
    *style = (TermStyle) {
        .fg = mask_color(style->fg, over->fg),
        .bg = has_default_bg ? mask_color(style->bg, over->bg) : style->bg,
        .attr = mask_attr(style->attr, over->attr),
    };
}

static void mask_selection_and_current_line (
    const StyleMap *styles,
    const LineInfo *info,
    TermStyle *style
) {
    if (info->offset >= info->sel_so && info->offset < info->sel_eo) {
        mask_style(style, &styles->builtin[BSE_SELECTION]);
    } else if (info->line_nr == info->view->cy) {
        int32_t default_bg = styles->builtin[BSE_DEFAULT].bg;
        mask_style2(style, &styles->builtin[BSE_CURRENTLINE], default_bg);
    }
}

static bool is_non_text(CodePoint u, bool display_special)
{
    if (u == '\t') {
        return display_special;
    }
    return u < 0x20 || u == 0x7F || u_is_unprintable(u);
}

static bool whitespace_error(const LineInfo *info, CodePoint u, size_t i)
{
    const View *view = info->view;
    WhitespaceErrorFlags flags = info->wse_flags;
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
        return !!(flags & mask);
    }

    if (!in_indent || !(flags & (WSE_SPACE_INDENT | WSE_SPACE_ALIGN))) {
        // Return early, without doing extra work, if WSE_SPACE_* checks
        // below aren't applicable
        return false;
    }

    const char *line = info->line;
    const size_t size = info->size;
    size_t pos = i;
    size_t count = 0;
    while (pos > 0 && line[pos - 1] == ' ') {
        pos--;
    }
    while (pos < size && line[pos] == ' ') {
        pos++;
        count++;
    }

    bool space_instead_of_tab = (count >= view->buffer->options.tab_width);
    bool space_before_tab = (pos < size && line[pos] == '\t');
    if (space_instead_of_tab || space_before_tab) {
        return !!(flags & WSE_SPACE_INDENT);
    }

    // Less than tab width spaces at end of indentation
    return !!(flags & WSE_SPACE_ALIGN);
}

static CodePoint screen_next_char (
    Terminal *term,
    LineInfo *info,
    const StyleMap *styles,
    bool display_special
) {
    bool ws_error;
    size_t count;
    size_t pos = info->pos;
    CodePoint u = (unsigned char)info->line[pos];

    if (likely(u < 0x80)) {
        info->pos++;
        count = 1;
        ws_error = (u == '\t' || u == ' ') && whitespace_error(info, u, pos);
    } else {
        u = u_get_nonascii(info->line, info->size, &info->pos);
        count = info->pos - pos;
        bool wse_special = (info->view->buffer->options.ws_error & WSE_SPECIAL);
        ws_error = wse_special && u_is_special_whitespace(u);
    }

    bool have_style = info->styles && info->styles[pos];
    TermStyle style = have_style ? *info->styles[pos] : styles->builtin[BSE_DEFAULT];

    if (is_non_text(u, display_special)) {
        mask_style(&style, &styles->builtin[BSE_NONTEXT]);
    }

    if (ws_error) {
        mask_style(&style, &styles->builtin[BSE_WSERROR]);
    }

    mask_selection_and_current_line(styles, info, &style);
    set_style(term, styles, &style);
    info->offset += count;
    return u;
}

static void screen_skip_char(TermOutputBuffer *obuf, LineInfo *info)
{
    CodePoint u = (unsigned char)info->line[info->pos++];
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
        return;
    }

    size_t pos = info->pos;
    info->pos--;
    u = u_get_nonascii(info->line, info->size, &info->pos);
    obuf->x += u_char_width(u);
    info->offset += info->pos - pos;
}

static bool is_notice(const char *word, size_t len)
{
    switch (len) {
        case 3: return mem_equal(word, STRN("XXX"));
        case 4: return mem_equal(word, STRN("TODO"));
        case 5: return mem_equal(word, STRN("FIXME"));
    }
    return false;
}

// Highlight certain words inside comments
static void hl_words (
    const LineInfo *info,
    const TermStyle *comment_style,
    const TermStyle *notice_style,
    unsigned int term_width
) {
    const TermStyle **styles = info->styles;
    if (!styles || !comment_style || !notice_style) {
        return;
    }

    const size_t size = info->size;
    size_t i = info->pos;
    if (i >= size) {
        return;
    }

    // Go to beginning of partially visible word inside comment
    const char *line = info->line;
    while (i > 0 && styles[i] == comment_style && is_word_byte(line[i])) {
        i--;
    }

    // This should be more than enough. I'm too lazy to iterate characters
    // instead of bytes and calculate text width.
    const size_t max = info->pos + (term_width * 4) + 8;

    while (i < size) {
        if (styles[i] != comment_style || !is_word_byte(line[i])) {
            if (i > max) {
                break;
            }
            i++;
            continue;
        }

        // Beginning of a word inside a comment
        size_t word_start = i++;

        // Move to the end of the word
        while (i < size) {
            if (styles[i] != comment_style || !is_word_byte(line[i])) {
                break;
            }
            i++;
        }

        // ...and highlight it, if applicable
        if (is_notice(line + word_start, i - word_start)) {
            set_style_range(styles, notice_style, word_start, i);
        }
    }
}

// Get effective `ws-error` flags (i.e. for `auto-indent`)
static WhitespaceErrorFlags get_wse_flags(const LocalOptions *opts)
{
    WhitespaceErrorFlags taberrs = WSE_TAB_INDENT | WSE_TAB_AFTER_INDENT;
    WhitespaceErrorFlags extra = opts->expand_tab ? taberrs : WSE_SPACE_INDENT;
    return opts->ws_error | ((opts->ws_error & WSE_AUTO_INDENT) ? extra : 0);
}

static LineInfo line_info_init(const View *view, const BlockIter *bi, size_t line_nr)
{
    LineInfo info = {
        .view = view,
        .line_nr = line_nr,
        .offset = block_iter_get_offset(bi),
        .wse_flags = get_wse_flags(&view->buffer->options),
        .sel_so = -1,
        .sel_eo = -1,
    };

    if (view->selection) {
        if (view->sel_eo == SEL_EO_RECALC) {
            SelectionInfo sel = init_selection(view);
            info.sel_so = sel.so;
            info.sel_eo = sel.eo;
        } else {
            // Already calculated
            info.sel_so = view->sel_so;
            info.sel_eo = view->sel_eo;
            BUG_ON(info.sel_so > info.sel_eo);
        }
    }

    return info;
}

static void line_info_set_line (
    LineInfo *info,
    StringView line,
    const TermStyle **styles
) {
    BUG_ON(line.length == 0);
    BUG_ON(line.data[line.length - 1] != '\n');

    info->line = line.data;
    info->size = line.length - 1;
    info->pos = 0;
    info->styles = styles;

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

static void print_line (
    Terminal *term,
    LineInfo *info,
    const StyleMap *styles,
    bool display_special
) {
    // Screen might be scrolled horizontally. Skip most invisible
    // characters using screen_skip_char(), which is much faster than
    // buf_skip(obuf, screen_next_char(...)).
    //
    // There can be a wide character (tab, control code etc.) that is
    // partially visible and can't be skipped using screen_skip_char().
    TermOutputBuffer *obuf = &term->obuf;
    while (obuf->x + 8 < obuf->scroll_x && info->pos < info->size) {
        screen_skip_char(obuf, info);
    }

    const TermStyle *comment = find_style(styles, "comment");
    const TermStyle *notice = find_style(styles, "notice");
    hl_words(info, comment, notice, term->width);

    while (info->pos < info->size) {
        BUG_ON(obuf->x > obuf->scroll_x + obuf->width);
        CodePoint u = screen_next_char(term, info, styles, display_special);
        if (!term_put_char(obuf, u)) {
            // +1 for newline
            info->offset += info->size - info->pos + 1;
            return;
        }
    }

    const TermStyle default_style = styles->builtin[BSE_DEFAULT];
    if (display_special && obuf->x >= obuf->scroll_x) {
        // Syntax highlighter highlights \n but use default style anyway
        TermStyle style = default_style;
        mask_style(&style, &styles->builtin[BSE_NONTEXT]);
        mask_selection_and_current_line(styles, info, &style);
        set_style(term, styles, &style);
        term_put_char(obuf, '$');
    }

    TermStyle style = default_style;
    mask_selection_and_current_line(styles, info, &style);
    set_style(term, styles, &style);
    info->offset++;
    term_clear_eol(term);
}

void update_range (
    Terminal *term,
    const View *view,
    const StyleMap *styles,
    long y1,
    long y2,
    bool display_special
) {
    const int edit_x = view->window->edit_x;
    const int edit_y = view->window->edit_y;
    const int edit_w = view->window->edit_w;
    const int edit_h = view->window->edit_h;

    TermOutputBuffer *obuf = &term->obuf;
    term_output_reset(term, edit_x, edit_w, view->vx);
    obuf->tab_width = view->buffer->options.tab_width;
    obuf->tab_mode = display_special ? TAB_SPECIAL : TAB_NORMAL;

    BlockIter bi = view->cursor;
    for (long i = 0, n = view->cy - y1; i < n; i++) {
        block_iter_prev_line(&bi);
    }
    for (long i = 0, n = y1 - view->cy; i < n; i++) {
        block_iter_eat_line(&bi);
    }
    block_iter_bol(&bi);

    LineInfo info = line_info_init(view, &bi, y1);

    y1 -= view->vy;
    y2 -= view->vy;

    bool got_line = !block_iter_is_eof(&bi);
    Syntax *syn = view->buffer->syntax;
    PointerArray *lss = &view->buffer->line_start_states;
    BlockIter tmp = block_iter(view->buffer);
    hl_fill_start_states(syn, lss, styles, &tmp, info.line_nr);
    long i;

    for (i = y1; got_line && i < y2; i++) {
        obuf->x = 0;
        term_move_cursor(obuf, edit_x, edit_y + i);

        StringView line = block_iter_get_line_with_nl(&bi);
        bool next_changed;
        const TermStyle **hlstyles = hl_line(syn, lss, styles, line, info.line_nr, &next_changed);
        line_info_set_line(&info, line, hlstyles);
        print_line(term, &info, styles, display_special);

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
        TermStyle style = styles->builtin[BSE_DEFAULT];
        mask_style2(&style, &styles->builtin[BSE_CURRENTLINE], style.bg);
        obuf->x = 0;
        set_style(term, styles, &style);
        term_move_cursor(obuf, edit_x, edit_y + i++);
        term_clear_eol(term);
    }

    if (i < y2) {
        set_builtin_style(term, styles, BSE_NOLINE);
    }

    for (; i < y2; i++) {
        obuf->x = 0;
        term_move_cursor(obuf, edit_x, edit_y + i);
        term_put_char(obuf, '~');
        term_clear_eol(term);
    }
}
