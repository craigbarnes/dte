#include "selection.h"
#include "editor.h"
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
