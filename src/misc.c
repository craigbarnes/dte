#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "buffer.h"
#include "change.h"
#include "indent.h"
#include "move.h"
#include "options.h"
#include "regexp.h"
#include "selection.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/string.h"
#include "util/string-view.h"
#include "util/utf8.h"

typedef struct {
    String buf;
    char *indent;
    size_t indent_len;
    size_t indent_width;
    size_t cur_width;
    size_t text_width;
} ParagraphFormatter;

static bool line_has_opening_brace(StringView line)
{
    static regex_t re;
    static bool compiled;
    if (!compiled) {
        // TODO: Reimplement without using regex
        static const char pat[] = "\\{[ \t]*(//.*|/\\*.*\\*/[ \t]*)?$";
        regexp_compile_or_fatal_error(&re, pat, REG_NEWLINE | REG_NOSUB);
        compiled = true;
    }

    regmatch_t m;
    return regexp_exec(&re, line.data, line.length, 0, &m, 0);
}

static bool line_has_closing_brace(StringView line)
{
    strview_trim_left(&line);
    return line.length > 0 && line.data[0] == '}';
}

/*
 * Stupid { ... } block selector.
 *
 * Because braces can be inside strings or comments and writing real
 * parser for many programming languages does not make sense the rules
 * for selecting a block are made very simple. Line that matches \{\s*$
 * starts a block and line that matches ^\s*\} ends it.
 */
void select_block(View *view)
{
    BlockIter sbi, ebi, bi = view->cursor;
    StringView line;
    int level = 0;

    // If current line does not match \{\s*$ but matches ^\s*\} then
    // cursor is likely at end of the block you want to select
    fetch_this_line(&bi, &line);
    if (!line_has_opening_brace(line) && line_has_closing_brace(line)) {
        block_iter_prev_line(&bi);
    }

    while (1) {
        fetch_this_line(&bi, &line);
        if (line_has_opening_brace(line)) {
            if (level++ == 0) {
                sbi = bi;
                block_iter_next_line(&bi);
                break;
            }
        }
        if (line_has_closing_brace(line)) {
            level--;
        }

        if (!block_iter_prev_line(&bi)) {
            return;
        }
    }

    while (1) {
        fetch_this_line(&bi, &line);
        if (line_has_closing_brace(line)) {
            if (--level == 0) {
                ebi = bi;
                break;
            }
        }
        if (line_has_opening_brace(line)) {
            level++;
        }

        if (!block_iter_next_line(&bi)) {
            return;
        }
    }

    view->cursor = sbi;
    view->sel_so = block_iter_get_offset(&ebi);
    view->sel_eo = SEL_EO_RECALC;
    view->selection = SELECT_LINES;

    mark_all_lines_changed(view->buffer);
}

static int get_indent_of_matching_brace(const View *view)
{
    const LocalOptions *options = &view->buffer->options;
    BlockIter bi = view->cursor;
    StringView line;
    int level = 0;

    while (block_iter_prev_line(&bi)) {
        fetch_this_line(&bi, &line);
        if (line_has_opening_brace(line)) {
            if (level++ == 0) {
                return get_indent_width(options, &line);
            }
        }
        if (line_has_closing_brace(line)) {
            level--;
        }
    }

    return -1;
}

void unselect(View *view)
{
    view->select_mode = SELECT_NONE;
    if (view->selection) {
        view->selection = SELECT_NONE;
        mark_all_lines_changed(view->buffer);
    }
}

void insert_text(View *view, const char *text, size_t size, bool move_after)
{
    size_t del_count = 0;
    if (view->selection) {
        del_count = prepare_selection(view);
        unselect(view);
    }
    buffer_replace_bytes(view, del_count, text, size);
    if (move_after) {
        block_iter_skip_bytes(&view->cursor, size);
    }
}

void delete_ch(View *view)
{
    size_t size = 0;
    if (view->selection) {
        size = prepare_selection(view);
        unselect(view);
    } else {
        const LocalOptions *options = &view->buffer->options;
        begin_change(CHANGE_MERGE_DELETE);
        if (options->emulate_tab) {
            size = get_indent_level_bytes_right(options, &view->cursor);
        }
        if (size == 0) {
            BlockIter bi = view->cursor;
            size = block_iter_next_column(&bi);
        }
    }
    buffer_delete_bytes(view, size);
}

void erase(View *view)
{
    size_t size = 0;
    if (view->selection) {
        size = prepare_selection(view);
        unselect(view);
    } else {
        const LocalOptions *options = &view->buffer->options;
        begin_change(CHANGE_MERGE_ERASE);
        if (options->emulate_tab) {
            size = get_indent_level_bytes_left(options, &view->cursor);
            block_iter_back_bytes(&view->cursor, size);
        }
        if (size == 0) {
            CodePoint u;
            size = block_iter_prev_char(&view->cursor, &u);
        }
    }
    buffer_erase_bytes(view, size);
}

// Go to beginning of whitespace (tabs and spaces) under cursor and
// return number of whitespace bytes after cursor after moving cursor
static size_t goto_beginning_of_whitespace(View *view)
{
    BlockIter bi = view->cursor;
    size_t count = 0;
    CodePoint u;

    // Count spaces and tabs at or after cursor
    while (block_iter_next_char(&bi, &u)) {
        if (u != '\t' && u != ' ') {
            break;
        }
        count++;
    }

    // Count spaces and tabs before cursor
    while (block_iter_prev_char(&view->cursor, &u)) {
        if (u != '\t' && u != ' ') {
            block_iter_next_char(&view->cursor, &u);
            break;
        }
        count++;
    }
    return count;
}

static bool ws_only(const StringView *line)
{
    for (size_t i = 0, n = line->length; i < n; i++) {
        char ch = line->data[i];
        if (ch != ' ' && ch != '\t') {
            return false;
        }
    }
    return true;
}

// Non-empty line can be used to determine size of indentation for the next line
static bool find_non_empty_line_bwd(BlockIter *bi)
{
    block_iter_bol(bi);
    do {
        StringView line;
        fill_line_ref(bi, &line);
        if (!ws_only(&line)) {
            return true;
        }
    } while (block_iter_prev_line(bi));
    return false;
}

static void insert_nl(View *view)
{
    size_t del_count = 0;
    size_t ins_count = 1;
    char *ins = NULL;

    // Prepare deleted text (selection or whitespace around cursor)
    if (view->selection) {
        del_count = prepare_selection(view);
        unselect(view);
    } else {
        // Trim whitespace around cursor
        del_count = goto_beginning_of_whitespace(view);
    }

    // Prepare inserted indentation
    const LocalOptions *options = &view->buffer->options;
    if (options->auto_indent) {
        // Current line will be split at cursor position
        BlockIter bi = view->cursor;
        size_t len = block_iter_bol(&bi);
        StringView line;
        fill_line_ref(&bi, &line);
        line.length = len;
        if (ws_only(&line)) {
            // This line is (or will become) white space only; find previous,
            // non whitespace only line
            if (block_iter_prev_line(&bi) && find_non_empty_line_bwd(&bi)) {
                fill_line_ref(&bi, &line);
                ins = get_indent_for_next_line(options, &line);
            }
        } else {
            ins = get_indent_for_next_line(options, &line);
        }
    }

    begin_change(CHANGE_MERGE_NONE);
    if (ins) {
        // Add newline before indent
        ins_count = strlen(ins);
        memmove(ins + 1, ins, ins_count);
        ins[0] = '\n';
        ins_count++;

        buffer_replace_bytes(view, del_count, ins, ins_count);
        free(ins);
    } else {
        buffer_replace_bytes(view, del_count, "\n", ins_count);
    }
    end_change();

    // Move after inserted text
    block_iter_skip_bytes(&view->cursor, ins_count);
}

void insert_ch(View *view, CodePoint ch)
{
    if (ch == '\n') {
        insert_nl(view);
        return;
    }

    const Buffer *buffer = view->buffer;
    const LocalOptions *options = &buffer->options;
    char buf[8];
    char *ins = buf;
    char *alloc = NULL;
    size_t del_count = 0;
    size_t ins_count = 0;

    if (view->selection) {
        // Prepare deleted text (selection)
        del_count = prepare_selection(view);
        unselect(view);
    } else if (options->overwrite) {
        // Delete character under cursor unless we're at end of line
        BlockIter bi = view->cursor;
        del_count = block_iter_is_eol(&bi) ? 0 : block_iter_next_column(&bi);
    } else if (ch == '}' && options->auto_indent && options->brace_indent) {
        BlockIter bi = view->cursor;
        StringView curlr;
        block_iter_bol(&bi);
        fill_line_ref(&bi, &curlr);
        if (ws_only(&curlr)) {
            int width = get_indent_of_matching_brace(view);
            if (width >= 0) {
                // Replace current (ws only) line with some indent + '}'
                block_iter_bol(&view->cursor);
                del_count = curlr.length;
                if (width) {
                    alloc = make_indent(options, width);
                    ins = alloc;
                    ins_count = strlen(ins);
                    // '}' will be replace the terminating NUL
                }
            }
        }
    }

    // Prepare inserted text
    if (ch == '\t' && options->expand_tab) {
        ins_count = options->indent_width;
        static_assert(sizeof(buf) >= INDENT_WIDTH_MAX);
        memset(ins, ' ', ins_count);
    } else {
        u_set_char_raw(ins, &ins_count, ch);
    }

    // Record change
    begin_change(del_count ? CHANGE_MERGE_NONE : CHANGE_MERGE_INSERT);
    buffer_replace_bytes(view, del_count, ins, ins_count);
    end_change();
    free(alloc);

    // Move after inserted text
    block_iter_skip_bytes(&view->cursor, ins_count);
}

static void join_selection(View *view)
{
    size_t count = prepare_selection(view);
    size_t len = 0, join = 0;
    BlockIter bi;
    CodePoint ch = 0;

    unselect(view);
    bi = view->cursor;

    begin_change_chain();
    while (count > 0) {
        if (!len) {
            view->cursor = bi;
        }

        count -= block_iter_next_char(&bi, &ch);
        if (ch == '\t' || ch == ' ') {
            len++;
        } else if (ch == '\n') {
            len++;
            join++;
        } else {
            if (join) {
                buffer_replace_bytes(view, len, " ", 1);
                // Skip the space we inserted and the char we read last
                block_iter_next_char(&view->cursor, &ch);
                block_iter_next_char(&view->cursor, &ch);
                bi = view->cursor;
            }
            len = 0;
            join = 0;
        }
    }

    // Don't replace last \n that is at end of the selection
    if (join && ch == '\n') {
        join--;
        len--;
    }

    if (join) {
        if (ch == '\n') {
            // Don't add space to end of line
            buffer_delete_bytes(view, len);
        } else {
            buffer_replace_bytes(view, len, " ", 1);
        }
    }
    end_change_chain(view);
}

void join_lines(View *view)
{
    BlockIter bi = view->cursor;

    if (view->selection) {
        join_selection(view);
        return;
    }

    if (!block_iter_next_line(&bi)) {
        return;
    }
    if (block_iter_is_eof(&bi)) {
        return;
    }

    BlockIter next = bi;
    CodePoint u;
    size_t count = 1;
    block_iter_prev_char(&bi, &u);
    while (block_iter_prev_char(&bi, &u)) {
        if (u != '\t' && u != ' ') {
            block_iter_next_char(&bi, &u);
            break;
        }
        count++;
    }
    while (block_iter_next_char(&next, &u)) {
        if (u != '\t' && u != ' ') {
            break;
        }
        count++;
    }

    view->cursor = bi;
    if (u == '\n') {
        buffer_delete_bytes(view, count);
    } else {
        buffer_replace_bytes(view, count, " ", 1);
    }
}

void clear_lines(View *view, bool auto_indent)
{
    char *indent = NULL;
    if (auto_indent) {
        BlockIter bi = view->cursor;
        if (block_iter_prev_line(&bi) && find_non_empty_line_bwd(&bi)) {
            StringView line;
            fill_line_ref(&bi, &line);
            indent = get_indent_for_next_line(&view->buffer->options, &line);
        }
    }

    size_t del_count = 0;
    if (view->selection) {
        view->selection = SELECT_LINES;
        del_count = prepare_selection(view);
        unselect(view);
        // Don't delete last newline
        if (del_count) {
            del_count--;
        }
    } else {
        block_iter_eol(&view->cursor);
        del_count = block_iter_bol(&view->cursor);
    }

    if (!indent && !del_count) {
        return;
    }

    size_t ins_count = indent ? strlen(indent) : 0;
    buffer_replace_bytes(view, del_count, indent, ins_count);
    free(indent);
    block_iter_skip_bytes(&view->cursor, ins_count);
}

void delete_lines(View *view)
{
    long x = view_get_preferred_x(view);
    size_t del_count;
    if (view->selection) {
        view->selection = SELECT_LINES;
        del_count = prepare_selection(view);
        unselect(view);
    } else {
        block_iter_bol(&view->cursor);
        BlockIter tmp = view->cursor;
        del_count = block_iter_eat_line(&tmp);
    }
    buffer_delete_bytes(view, del_count);
    move_to_preferred_x(view, x);
}

void new_line(View *view, bool above)
{
    if (above && block_iter_prev_line(&view->cursor) == 0) {
        // Already on first line; insert newline at bof
        block_iter_bol(&view->cursor);
        buffer_insert_bytes(view, "\n", 1);
        return;
    }

    const LocalOptions *options = &view->buffer->options;
    char *ins = NULL;
    block_iter_eol(&view->cursor);

    if (options->auto_indent) {
        BlockIter bi = view->cursor;
        if (find_non_empty_line_bwd(&bi)) {
            StringView line;
            fill_line_ref(&bi, &line);
            ins = get_indent_for_next_line(options, &line);
        }
    }

    size_t ins_count;
    if (ins) {
        ins_count = strlen(ins);
        memmove(ins + 1, ins, ins_count);
        ins[0] = '\n';
        ins_count++;
        buffer_insert_bytes(view, ins, ins_count);
        free(ins);
    } else {
        ins_count = 1;
        buffer_insert_bytes(view, "\n", 1);
    }

    block_iter_skip_bytes(&view->cursor, ins_count);
}

static void add_word(ParagraphFormatter *pf, const char *word, size_t len)
{
    size_t i = 0;
    size_t word_width = 0;
    while (i < len) {
        word_width += u_char_width(u_get_char(word, len, &i));
    }

    if (pf->cur_width && pf->cur_width + 1 + word_width > pf->text_width) {
        string_append_byte(&pf->buf, '\n');
        pf->cur_width = 0;
    }

    if (pf->cur_width == 0) {
        if (pf->indent_len) {
            string_append_buf(&pf->buf, pf->indent, pf->indent_len);
        }
        pf->cur_width = pf->indent_width;
    } else {
        string_append_byte(&pf->buf, ' ');
        pf->cur_width++;
    }

    string_append_buf(&pf->buf, word, len);
    pf->cur_width += word_width;
}

static bool is_paragraph_separator(const StringView *line)
{
    StringView trimmed = *line;
    strview_trim(&trimmed);

    return
        trimmed.length == 0
        // TODO: make this configurable
        || strview_equal_cstring(&trimmed, "/*")
        || strview_equal_cstring(&trimmed, "*/")
    ;
}

static bool in_paragraph(const LocalOptions *options, const StringView *line, size_t indent_width)
{
    if (get_indent_width(options, line) != indent_width) {
        return false;
    }
    return !is_paragraph_separator(line);
}

static size_t paragraph_size(View *view)
{
    const LocalOptions *options = &view->buffer->options;
    BlockIter bi = view->cursor;
    StringView line;
    block_iter_bol(&bi);
    fill_line_ref(&bi, &line);
    if (is_paragraph_separator(&line)) {
        // Not in paragraph
        return 0;
    }
    size_t indent_width = get_indent_width(options, &line);

    // Go to beginning of paragraph
    while (block_iter_prev_line(&bi)) {
        fill_line_ref(&bi, &line);
        if (!in_paragraph(options, &line, indent_width)) {
            block_iter_eat_line(&bi);
            break;
        }
    }
    view->cursor = bi;

    // Get size of paragraph
    size_t size = 0;
    do {
        size_t bytes = block_iter_eat_line(&bi);
        if (!bytes) {
            break;
        }
        size += bytes;
        fill_line_ref(&bi, &line);
    } while (in_paragraph(options, &line, indent_width));
    return size;
}

void format_paragraph(View *view, size_t text_width)
{
    size_t len;
    if (view->selection) {
        view->selection = SELECT_LINES;
        len = prepare_selection(view);
    } else {
        len = paragraph_size(view);
    }
    if (!len) {
        return;
    }

    const LocalOptions *options = &view->buffer->options;
    char *sel = block_iter_get_bytes(&view->cursor, len);
    StringView sv = string_view(sel, len);
    size_t indent_width = get_indent_width(options, &sv);
    char *indent = make_indent(options, indent_width);

    ParagraphFormatter pf = {
        .buf = STRING_INIT,
        .indent = indent,
        .indent_len = indent ? strlen(indent) : 0,
        .indent_width = indent_width,
        .cur_width = 0,
        .text_width = text_width
    };

    for (size_t i = 0; true; ) {
        while (i < len) {
            size_t tmp = i;
            if (!u_is_breakable_whitespace(u_get_char(sel, len, &tmp))) {
                break;
            }
            i = tmp;
        }
        if (i == len) {
            break;
        }

        size_t start = i;
        while (i < len) {
            size_t tmp = i;
            if (u_is_breakable_whitespace(u_get_char(sel, len, &tmp))) {
                break;
            }
            i = tmp;
        }

        add_word(&pf, sel + start, i - start);
    }

    if (pf.buf.len) {
        string_append_byte(&pf.buf, '\n');
    }
    buffer_replace_bytes(view, len, pf.buf.buffer, pf.buf.len);
    if (pf.buf.len) {
        block_iter_skip_bytes(&view->cursor, pf.buf.len - 1);
    }
    string_free(&pf.buf);
    free(pf.indent);
    free(sel);

    unselect(view);
}

void change_case(View *view, char mode)
{
    bool was_selecting = false;
    bool move = true;
    size_t text_len;
    if (view->selection) {
        SelectionInfo info;
        init_selection(view, &info);
        view->cursor = info.si;
        text_len = info.eo - info.so;
        unselect(view);
        was_selecting = true;
        move = !info.swapped;
    } else {
        CodePoint u;
        if (!block_iter_get_char(&view->cursor, &u)) {
            return;
        }
        text_len = u_char_size(u);
    }

    String dst = string_new(text_len);
    char *src = block_iter_get_bytes(&view->cursor, text_len);
    size_t i = 0;
    switch (mode) {
    case 'l':
        while (i < text_len) {
            CodePoint u = u_to_lower(u_get_char(src, text_len, &i));
            string_append_codepoint(&dst, u);
        }
        break;
    case 'u':
        while (i < text_len) {
            CodePoint u = u_to_upper(u_get_char(src, text_len, &i));
            string_append_codepoint(&dst, u);
        }
        break;
    case 't':
        while (i < text_len) {
            CodePoint u = u_get_char(src, text_len, &i);
            u = u_is_upper(u) ? u_to_lower(u) : u_to_upper(u);
            string_append_codepoint(&dst, u);
        }
        break;
    default:
        BUG("unhandled case mode");
    }

    buffer_replace_bytes(view, text_len, dst.buffer, dst.len);
    free(src);

    if (move && dst.len > 0) {
        if (was_selecting) {
            // Move cursor back to where it was
            size_t idx = dst.len;
            u_prev_char(dst.buffer, &idx);
            block_iter_skip_bytes(&view->cursor, idx);
        } else {
            block_iter_skip_bytes(&view->cursor, dst.len);
        }
    }

    string_free(&dst);
}
