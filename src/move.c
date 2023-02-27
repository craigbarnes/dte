#include "move.h"
#include "buffer.h"
#include "indent.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/utf8.h"

typedef enum {
    CT_SPACE,
    CT_NEWLINE,
    CT_WORD,
    CT_OTHER,
} CharTypeEnum;

void move_to_preferred_x(View *view, long preferred_x)
{
    const LocalOptions *options = &view->buffer->options;
    StringView line;
    view->preferred_x = preferred_x;
    block_iter_bol(&view->cursor);
    fill_line_ref(&view->cursor, &line);

    if (options->emulate_tab && view->preferred_x < line.length) {
        const size_t iw = options->indent_width;
        const size_t ilevel = indent_level(view->preferred_x, iw);
        for (size_t i = 0; i < line.length && line.data[i] == ' '; i++) {
            if (i + 1 == (ilevel + 1) * iw) {
                // Force cursor to beginning of the indentation level
                view->cursor.offset += ilevel * iw;
                return;
            }
        }
    }

    const unsigned int tw = options->tab_width;
    unsigned long x = 0;
    size_t i = 0;
    while (x < view->preferred_x && i < line.length) {
        CodePoint u = line.data[i++];
        if (likely(u < 0x80)) {
            if (likely(!ascii_iscntrl(u))) {
                x++;
            } else if (u == '\t') {
                x = next_indent_width(x, tw);
            } else if (u == '\n') {
                break;
            } else {
                x += 2;
            }
        } else {
            const size_t next = i;
            i--;
            u = u_get_nonascii(line.data, line.length, &i);
            x += u_char_width(u);
            if (x > view->preferred_x) {
                i = next;
                break;
            }
        }
    }
    if (x > view->preferred_x) {
        i--;
    }
    view->cursor.offset += i;

    // If cursor stopped on a zero-width char, move to the next spacing char
    CodePoint u;
    if (block_iter_get_char(&view->cursor, &u) && u_is_zero_width(u)) {
        block_iter_next_column(&view->cursor);
    }
}

void move_cursor_left(View *view)
{
    const LocalOptions *options = &view->buffer->options;
    if (options->emulate_tab) {
        size_t size = get_indent_level_bytes_left(options, &view->cursor);
        if (size) {
            block_iter_back_bytes(&view->cursor, size);
            view_reset_preferred_x(view);
            return;
        }
    }
    block_iter_prev_column(&view->cursor);
    view_reset_preferred_x(view);
}

void move_cursor_right(View *view)
{
    const LocalOptions *options = &view->buffer->options;
    if (options->emulate_tab) {
        size_t size = get_indent_level_bytes_right(options, &view->cursor);
        if (size) {
            block_iter_skip_bytes(&view->cursor, size);
            view_reset_preferred_x(view);
            return;
        }
    }
    block_iter_next_column(&view->cursor);
    view_reset_preferred_x(view);
}

void move_bol(View *view)
{
    block_iter_bol(&view->cursor);
    view_reset_preferred_x(view);
}

void move_bol_smart(View *view, SmartBolFlags flags)
{
    if (flags == 0) {
        move_bol(view);
        return;
    }

    BUG_ON(!(flags & BOL_SMART));
    bool fwd = false;
    StringView line;
    size_t cursor_offset = fetch_this_line(&view->cursor, &line);

    if (cursor_offset == 0) {
        // Already at bol
        if (!(flags & BOL_SMART_TOGGLE)) {
            goto out;
        }
        fwd = true;
    }

    size_t indent = ascii_blank_prefix_length(line.data, line.length);
    if (fwd) {
        block_iter_skip_bytes(&view->cursor, indent);
    } else {
        size_t co = cursor_offset;
        size_t move = (co > indent) ? co - indent : co;
        block_iter_back_bytes(&view->cursor, move);
    }

out:
    view_reset_preferred_x(view);
}

void move_eol(View *view)
{
    block_iter_eol(&view->cursor);
    view_reset_preferred_x(view);
}

void move_up(View *view, long count)
{
    const long x = view_get_preferred_x(view);
    while (count > 0) {
        if (!block_iter_prev_line(&view->cursor)) {
            break;
        }
        count--;
    }
    move_to_preferred_x(view, x);
}

void move_down(View *view, long count)
{
    const long x = view_get_preferred_x(view);
    while (count > 0) {
        if (!block_iter_eat_line(&view->cursor)) {
            break;
        }
        count--;
    }
    move_to_preferred_x(view, x);
}

void move_bof(View *view)
{
    block_iter_bof(&view->cursor);
    view_reset_preferred_x(view);
}

void move_eof(View *view)
{
    block_iter_eof(&view->cursor);
    view_reset_preferred_x(view);
}

void move_to_line(View *view, size_t line)
{
    BUG_ON(line == 0);
    view->center_on_scroll = true;
    block_iter_goto_line(&view->cursor, line - 1);
}

void move_to_column(View *view, size_t column)
{
    BUG_ON(column == 0);
    block_iter_bol(&view->cursor);
    while (column-- > 1) {
        CodePoint u;
        if (!block_iter_next_char(&view->cursor, &u)) {
            break;
        }
        if (u == '\n') {
            block_iter_prev_char(&view->cursor, &u);
            break;
        }
    }
    view_reset_preferred_x(view);
}

void move_to_filepos(View *view, size_t line, size_t column)
{
    move_to_line(view, line);
    BUG_ON(!block_iter_is_bol(&view->cursor));
    if (column != 1) {
        move_to_column(view, column);
    }
    view_reset_preferred_x(view);
}

static CharTypeEnum get_char_type(CodePoint u)
{
    if (u == '\n') {
        return CT_NEWLINE;
    }
    if (u_is_breakable_whitespace(u)) {
        return CT_SPACE;
    }
    if (u_is_word_char(u)) {
        return CT_WORD;
    }
    return CT_OTHER;
}

static bool get_current_char_type(BlockIter *bi, CharTypeEnum *type)
{
    CodePoint u;
    if (!block_iter_get_char(bi, &u)) {
        return false;
    }

    *type = get_char_type(u);
    return true;
}

static size_t skip_fwd_char_type(BlockIter *bi, CharTypeEnum type)
{
    size_t count = 0;
    CodePoint u;
    while (block_iter_next_char(bi, &u)) {
        if (get_char_type(u) != type) {
            block_iter_prev_char(bi, &u);
            break;
        }
        count += u_char_size(u);
    }
    return count;
}

static size_t skip_bwd_char_type(BlockIter *bi, CharTypeEnum type)
{
    size_t count = 0;
    CodePoint u;
    while (block_iter_prev_char(bi, &u)) {
        if (get_char_type(u) != type) {
            block_iter_next_char(bi, &u);
            break;
        }
        count += u_char_size(u);
    }
    return count;
}

size_t word_fwd(BlockIter *bi, bool skip_non_word)
{
    size_t count = 0;
    CharTypeEnum type;

    while (1) {
        count += skip_fwd_char_type(bi, CT_SPACE);
        if (!get_current_char_type(bi, &type)) {
            return count;
        }

        if (
            count
            && (!skip_non_word || (type == CT_WORD || type == CT_NEWLINE))
        ) {
            return count;
        }

        count += skip_fwd_char_type(bi, type);
    }
}

size_t word_bwd(BlockIter *bi, bool skip_non_word)
{
    size_t count = 0;
    CharTypeEnum type;
    CodePoint u;

    do {
        count += skip_bwd_char_type(bi, CT_SPACE);
        if (!block_iter_prev_char(bi, &u)) {
            return count;
        }

        type = get_char_type(u);
        count += u_char_size(u);
        count += skip_bwd_char_type(bi, type);
    } while (skip_non_word && type != CT_WORD && type != CT_NEWLINE);
    return count;
}
