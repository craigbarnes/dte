#include <stdlib.h>
#include <string.h>
#include "insert.h"
#include "block-iter.h"
#include "buffer.h"
#include "change.h"
#include "indent.h"
#include "options.h"
#include "selection.h"
#include "util/utf8.h"

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

static size_t insert_nl_and_autoindent (
    View *view,
    const StringView *prev_line,
    size_t del_count
) {
    const char *ins = "\n";
    size_t ins_count = 1;
    char *indent = NULL;

    if (prev_line->length) {
        indent = get_indent_for_next_line(&view->buffer->options, prev_line);
        if (indent) {
            ins_count = strlen(indent);
            memmove(indent + 1, indent, ins_count++);
            indent[0] = '\n';
            ins = indent;
        }
    }

    buffer_replace_bytes(view, del_count, ins, ins_count);
    free(indent);
    return ins_count;
}

void new_line(View *view, bool auto_indent, bool above_cursor)
{
    if (above_cursor && block_iter_prev_line(&view->cursor) == 0) {
        // Already on first line; insert newline at bof
        block_iter_bol(&view->cursor);
        buffer_insert_bytes(view, "\n", 1);
        return;
    }

    block_iter_eol(&view->cursor);
    BlockIter tmp = view->cursor;
    StringView line = STRING_VIEW_INIT;

    if (auto_indent && block_iter_find_non_empty_line_bwd(&tmp)) {
        line = block_iter_get_line(&tmp);
    }

    size_t ins_count = insert_nl_and_autoindent(view, &line, 0);
    block_iter_skip_bytes(&view->cursor, ins_count);
}

// Go to beginning of whitespace (tabs and spaces) under cursor and
// return number of whitespace bytes surrounding cursor
static inline size_t goto_beginning_of_whitespace(BlockIter *cursor)
{
    BlockIter tmp = *cursor;
    return block_iter_skip_blanks_fwd(&tmp) + block_iter_skip_blanks_bwd(cursor);
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

    // Get reference line, for caculating auto-indent size (if applicable)
    StringView line = STRING_VIEW_INIT;
    if (view->buffer->options.auto_indent) {
        BlockIter bi = view->cursor;
        size_t len = block_iter_bol(&bi);
        line = block_iter_get_line(&bi);
        line.length = len; // Current line will be split at cursor position
        if (strview_isblank(&line)) {
            // This line is (or will become) whitespace only; find previous,
            // non-whitespace line
            if (block_iter_prev_line(&bi) && block_iter_find_non_empty_line_bwd(&bi)) {
                line = block_iter_get_line(&bi);
            } else {
                line.length = 0;
            }
        }
    }

    begin_change(CHANGE_MERGE_NONE);
    size_t ins_count = insert_nl_and_autoindent(view, &line, del_count);
    end_change();
    block_iter_skip_bytes(&view->cursor, ins_count); // Move after inserted text
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
        // Prepare text to be deleted (selection)
        del_count = prepare_selection(view);
        unselect(view);
    } else if (options->overwrite) {
        // Delete character under cursor unless we're at end of line
        BlockIter bi = view->cursor;
        del_count = block_iter_is_eol(&bi) ? 0 : block_iter_next_column(&bi);
    } else if (ch == '}' && options->auto_indent && options->brace_indent) {
        StringView curlr;
        fetch_this_line(&view->cursor, &curlr);
        if (strview_isblank(&curlr)) {
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

    // Prepare text to be inserted
    if (ch == '\t' && options->expand_tab) {
        static_assert(sizeof(buf) >= INDENT_WIDTH_MAX);
        ins_count = options->indent_width;
        memset(ins, ' ', ins_count);
    } else {
        static_assert(sizeof(buf) >= UTF8_MAX_SEQ_LEN);
        ins_count += u_set_char_raw(ins + ins_count, ch);
    }

    // Make edit to Buffer (and record Change in undo history)
    begin_change(del_count ? CHANGE_MERGE_NONE : CHANGE_MERGE_INSERT);
    buffer_replace_bytes(view, del_count, ins, ins_count);
    end_change();
    free(alloc);

    // Move cursor after inserted text
    block_iter_skip_bytes(&view->cursor, ins_count);
}
