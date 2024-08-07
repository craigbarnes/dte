#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "buffer.h"
#include "change.h"
#include "indent.h"
#include "options.h"
#include "regexp.h"
#include "selection.h"
#include "util/ascii.h"
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
    BlockIter bi = view->cursor;
    StringView line;
    fetch_this_line(&bi, &line);

    // If current line does not match \{\s*$ but matches ^\s*\} then
    // cursor is likely at end of the block you want to select
    if (!line_has_opening_brace(line) && line_has_closing_brace(line)) {
        block_iter_prev_line(&bi);
    }

    BlockIter sbi;
    int level = 0;
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

    BlockIter ebi;
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
    unsigned int tab_width = view->buffer->options.tab_width;
    BlockIter bi = view->cursor;
    StringView line;
    int level = 0;

    while (block_iter_prev_line(&bi)) {
        fetch_this_line(&bi, &line);
        if (line_has_opening_brace(line)) {
            if (level++ == 0) {
                return get_indent_width(&line, tab_width);
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

// Count spaces and tabs at or after iterator (and move beyond them)
static size_t count_blanks_fwd(BlockIter *bi)
{
    block_iter_normalize(bi);
    const char *data = bi->blk->data;
    size_t count = 0;
    size_t i = bi->offset;

    // We're only operating on one line and checking for ASCII characters,
    // so Block traversal and Unicode-aware decoding are both unnecessary
    for (size_t n = bi->blk->size; i < n; count++) {
        unsigned char c = data[i++];
        if (!ascii_isblank(c)) {
            break;
        }
    }

    bi->offset = i;
    return count;
}

// Count spaces and tabs before iterator (and move to beginning of them)
static size_t count_blanks_bwd(BlockIter *bi)
{
    block_iter_normalize(bi);
    size_t count = 0;
    size_t i = bi->offset;

    for (const char *data = bi->blk->data; i > 0; count++) {
        unsigned char c = data[--i];
        if (!ascii_isblank(c)) {
            i++;
            break;
        }
    }

    bi->offset = i;
    return count;
}

// Go to beginning of whitespace (tabs and spaces) under cursor and
// return number of whitespace bytes surrounding cursor
static size_t goto_beginning_of_whitespace(BlockIter *cursor)
{
    BlockIter tmp = *cursor;
    return count_blanks_fwd(&tmp) + count_blanks_bwd(cursor);
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
        StringView line = block_iter_get_line(bi);
        if (!ws_only(&line)) {
            return true;
        }
    } while (block_iter_prev_line(bi));
    return false;
}

static void insert_nl(View *view)
{
    // Prepare deleted text (selection or whitespace around cursor)
    size_t del_count = 0;
    if (view->selection) {
        del_count = prepare_selection(view);
        unselect(view);
    } else {
        // Trim whitespace around cursor
        del_count = goto_beginning_of_whitespace(&view->cursor);
    }

    // Prepare inserted indentation
    const LocalOptions *options = &view->buffer->options;
    char *ins = NULL;
    if (options->auto_indent) {
        // Current line will be split at cursor position
        BlockIter bi = view->cursor;
        size_t len = block_iter_bol(&bi);
        StringView line = block_iter_get_line(&bi);
        line.length = len;
        if (ws_only(&line)) {
            // This line is (or will become) white space only; find previous,
            // non whitespace only line
            if (block_iter_prev_line(&bi) && find_non_empty_line_bwd(&bi)) {
                line = block_iter_get_line(&bi);
                ins = get_indent_for_next_line(options, &line);
            }
        } else {
            ins = get_indent_for_next_line(options, &line);
        }
    }

    size_t ins_count;
    begin_change(CHANGE_MERGE_NONE);

    if (ins) {
        ins_count = strlen(ins);
        memmove(ins + 1, ins, ins_count);
        ins[0] = '\n'; // Add newline before indent
        ins_count++;
        buffer_replace_bytes(view, del_count, ins, ins_count);
        free(ins);
    } else {
        ins_count = 1;
        buffer_replace_bytes(view, del_count, "\n", ins_count);
    }

    end_change();
    block_iter_skip_bytes(&view->cursor, ins_count); // Move after inserted text
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
        StringView curlr;
        fetch_this_line(&view->cursor, &curlr);
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
        ins_count += u_set_char_raw(ins + ins_count, ch);
    }

    // Record change
    begin_change(del_count ? CHANGE_MERGE_NONE : CHANGE_MERGE_INSERT);
    buffer_replace_bytes(view, del_count, ins, ins_count);
    end_change();
    free(alloc);

    // Move after inserted text
    block_iter_skip_bytes(&view->cursor, ins_count);
}

static void join_selection(View *view, const char *delim, size_t delim_len)
{
    size_t count = prepare_selection(view);
    BlockIter bi = view->cursor;
    size_t ws_len = 0;
    size_t join = 0;
    CodePoint ch = 0;
    unselect(view);
    begin_change_chain();

    while (count > 0) {
        if (!ws_len) {
            view->cursor = bi;
        }

        size_t n = block_iter_next_char(&bi, &ch);
        count -= MIN(n, count);
        if (ch == '\n' || ch == '\t' || ch == ' ') {
            join += (ch == '\n');
            ws_len++;
            continue;
        }

        if (join) {
            buffer_replace_bytes(view, ws_len, delim, delim_len);
            // Skip the delimiter we inserted and the char we read last
            block_iter_skip_bytes(&view->cursor, delim_len);
            block_iter_next_char(&view->cursor, &ch);
            bi = view->cursor;
        }

        ws_len = 0;
        join = 0;
    }

    if (join && ch == '\n') {
        // Don't replace last newline at end of selection
        join--;
        ws_len--;
    }

    if (join) {
        size_t ins_len = (ch == '\n') ? 0 : delim_len; // Don't add delim, if at eol
        buffer_replace_bytes(view, ws_len, delim, ins_len);
    }

    end_change_chain(view);
}

void join_lines(View *view, const char *delim, size_t delim_len)
{
    if (view->selection) {
        join_selection(view, delim, delim_len);
        return;
    }

    // Create an iterator and position it at the beginning of the next line
    // (or return early, if there is no next line)
    BlockIter next = view->cursor;
    if (!block_iter_next_line(&next) || block_iter_is_eof(&next)) {
        return;
    }

    // Create a second iterator and position it at the end of the current line
    BlockIter eol = next;
    CodePoint u;
    size_t nbytes = block_iter_prev_char(&eol, &u);
    BUG_ON(nbytes != 1);
    BUG_ON(u != '\n');

    // Skip over trailing whitespace at the end of the current line
    size_t del_count = 1 + count_blanks_bwd(&eol);

    // Skip over leading whitespace at the start of the next line
    del_count += count_blanks_fwd(&next);

    // Move the cursor to the join position
    view->cursor = eol;

    if (block_iter_is_bol(&next)) {
        // If the next line is empty (or whitespace only) just discard
        // it, by deleting the newline and any whitespace
        buffer_delete_bytes(view, del_count);
    } else {
        // Otherwise, join the current and next lines together, by
        // replacing the newline/whitespace with the delimiter string
        buffer_replace_bytes(view, del_count, delim, delim_len);
    }
}

void clear_lines(View *view, bool auto_indent)
{
    char *indent = NULL;
    if (auto_indent) {
        BlockIter bi = view->cursor;
        if (block_iter_prev_line(&bi) && find_non_empty_line_bwd(&bi)) {
            StringView line = block_iter_get_line(&bi);
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
            StringView line = block_iter_get_line(&bi);
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

static bool is_long_comment_delim(const StringView *sv)
{
    // TODO: make this configurable
    return strview_equal_cstring(sv, "/*") || strview_equal_cstring(sv, "*/");
}

static bool is_paragraph_separator(const StringView *line)
{
    StringView trimmed = *line;
    strview_trim(&trimmed);
    return (trimmed.length == 0) || is_long_comment_delim(&trimmed);
}

static bool in_paragraph (
    const StringView *line,
    size_t para_indent_width,
    unsigned int tab_width
) {
    size_t w = get_indent_width(line, tab_width);
    return (w == para_indent_width) && !is_paragraph_separator(line);
}

static size_t paragraph_size(View *view)
{
    BlockIter bi = view->cursor;
    block_iter_bol(&bi);
    StringView line = block_iter_get_line(&bi);
    if (is_paragraph_separator(&line)) {
        // Not in paragraph
        return 0;
    }

    unsigned int tab_width = view->buffer->options.tab_width;
    size_t para_indent_width = get_indent_width(&line, tab_width);

    // Go to beginning of paragraph
    while (block_iter_prev_line(&bi)) {
        line = block_iter_get_line(&bi);
        if (!in_paragraph(&line, para_indent_width, tab_width)) {
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
        line = block_iter_get_line(&bi);
    } while (in_paragraph(&line, para_indent_width, tab_width));
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
    size_t indent_width = get_indent_width(&sv, options->tab_width);
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
        SelectionInfo info = init_selection(view);
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
        size_t skip = dst.len;
        if (was_selecting) {
            // Move cursor back to where it was
            u_prev_char(dst.buffer, &skip);
        }
        block_iter_skip_bytes(&view->cursor, skip);
    }

    string_free(&dst);
}
