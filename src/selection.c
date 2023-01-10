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

void init_selection(const View *view, SelectionInfo *info)
{
    info->so = view->sel_so;
    info->eo = block_iter_get_offset(&view->cursor);
    info->si = view->cursor;
    block_iter_goto_offset(&info->si, info->so);
    info->swapped = false;
    if (info->so > info->eo) {
        size_t o = info->so;
        info->so = info->eo;
        info->eo = o;
        info->si = view->cursor;
        info->swapped = true;
    }

    BlockIter ei = info->si;
    block_iter_skip_bytes(&ei, info->eo - info->so);
    if (block_iter_is_eof(&ei)) {
        if (info->so == info->eo) {
            return;
        }
        CodePoint u;
        info->eo -= block_iter_prev_char(&ei, &u);
    }

    if (view->selection == SELECT_LINES) {
        info->so -= block_iter_bol(&info->si);
        info->eo += block_iter_eat_line(&ei);
    } else {
        if (include_cursor_char_in_selection(view)) {
            info->eo += block_iter_next_column(&ei);
        }
    }
}

size_t prepare_selection(View *view)
{
    SelectionInfo info;
    init_selection(view, &info);
    view->cursor = info.si;
    return info.eo - info.so;
}

char *view_get_selection(View *view, size_t *size)
{
    if (view->selection == SELECT_NONE) {
        *size = 0;
        return NULL;
    }

    BlockIter save = view->cursor;
    *size = prepare_selection(view);
    char *buf = block_iter_get_bytes(&view->cursor, *size);
    view->cursor = save;
    return buf;
}

size_t get_nr_selected_lines(const SelectionInfo *info)
{
    BlockIter bi = info->si;
    size_t pos = info->so;
    CodePoint u = 0;
    size_t nr_lines = 1;

    while (pos < info->eo) {
        if (u == '\n') {
            nr_lines++;
        }
        pos += block_iter_next_char(&bi, &u);
    }
    return nr_lines;
}

size_t get_nr_selected_chars(const SelectionInfo *info)
{
    BlockIter bi = info->si;
    size_t pos = info->so;
    CodePoint u;
    size_t nr_chars = 0;

    while (pos < info->eo) {
        nr_chars++;
        pos += block_iter_next_char(&bi, &u);
    }
    return nr_chars;
}
