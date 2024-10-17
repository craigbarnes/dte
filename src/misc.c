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

bool line_has_opening_brace(StringView line)
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

bool line_has_closing_brace(StringView line)
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

void unselect(View *view)
{
    view->select_mode = SELECT_NONE;
    if (view->selection) {
        view->selection = SELECT_NONE;
        mark_all_lines_changed(view->buffer);
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
    size_t del_count = 1 + block_iter_skip_blanks_bwd(&eol);

    // Skip over leading whitespace at the start of the next line
    del_count += block_iter_skip_blanks_fwd(&next);

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
        if (block_iter_prev_line(&bi) && block_iter_find_non_empty_line_bwd(&bi)) {
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
