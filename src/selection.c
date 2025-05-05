#include "selection.h"
#include "editor.h"
#include "regexp.h"
#include "util/unicode.h"

static bool include_cursor_char_in_selection(const View *view)
{
    const EditorState *e = view->window->editor;
    if (!e->options.select_cursor_char) {
        return false;
    }

    bool overwrite = view->buffer->options.overwrite;
    CursorInputMode mode = overwrite ? CURSOR_MODE_OVERWRITE : CURSOR_MODE_INSERT;
    TermCursorType type = e->cursor_styles[mode].type;
    if (type == CURSOR_KEEP) {
        type = e->cursor_styles[CURSOR_MODE_DEFAULT].type;
    }

    // If "select-cursor-char" option is true, include character under cursor
    // in selections for any cursor type except bars (where it makes no sense
    // to do so)
    return !(type == CURSOR_STEADY_BAR || type == CURSOR_BLINKING_BAR);
}

SelectionInfo init_selection(const View *view)
{
    size_t so = view->sel_so;
    size_t eo = block_iter_get_offset(&view->cursor);
    bool swapped = (so > eo);

    SelectionInfo info = {
        .si = view->cursor,
        .so = swapped ? eo : so,
        .eo = swapped ? so : eo,
        .swapped = swapped,
    };

    if (!swapped) {
        block_iter_goto_offset(&info.si, so);
    }

    BlockIter ei = info.si;
    block_iter_skip_bytes(&ei, info.eo - info.so);
    if (block_iter_is_eof(&ei)) {
        if (info.so == info.eo) {
            return info;
        }
        CodePoint u;
        info.eo -= block_iter_prev_char(&ei, &u);
    }

    if (view->selection == SELECT_LINES) {
        info.so -= block_iter_bol(&info.si);
        info.eo += block_iter_eat_line(&ei);
    } else if (include_cursor_char_in_selection(view)) {
        info.eo += block_iter_next_column(&ei);
    }

    return info;
}

size_t prepare_selection(View *view)
{
    SelectionInfo info = init_selection(view);
    view->cursor = info.si;
    return info.eo - info.so;
}

size_t get_nr_selected_lines(const SelectionInfo *info)
{
    BlockIter bi = info->si;
    size_t nr_lines = 0;

    for (size_t pos = info->so, eo = info->eo; pos < eo; nr_lines++) {
        pos += block_iter_eat_line(&bi);
        BUG_ON(block_iter_is_eof(&bi) && pos != info->eo);
    }

    return nr_lines;
}

size_t get_nr_selected_chars(const SelectionInfo *info)
{
    BlockIter bi = info->si;
    size_t nr_chars = 0;
    CodePoint u;

    for (size_t pos = info->so, eo = info->eo; pos < eo; nr_chars++) {
        pos += block_iter_next_char(&bi, &u);
    }

    return nr_chars;
}

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
    StringView line = get_current_line(&bi);

    // If current line does not match \{\s*$ but matches ^\s*\} then
    // cursor is likely at end of the block you want to select
    if (!line_has_opening_brace(line) && line_has_closing_brace(line)) {
        block_iter_prev_line(&bi);
    }

    BlockIter sbi;
    int level = 0;
    while (1) {
        line = get_current_line(&bi);
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
        line = get_current_line(&bi);
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
