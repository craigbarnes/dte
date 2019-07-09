#include "selection.h"
#include "buffer.h"

void init_selection(const View *v, SelectionInfo *info)
{
    BlockIter ei;
    CodePoint u;

    info->so = v->sel_so;
    info->eo = block_iter_get_offset(&v->cursor);
    info->si = v->cursor;
    block_iter_goto_offset(&info->si, info->so);
    info->swapped = false;
    if (info->so > info->eo) {
        size_t o = info->so;
        info->so = info->eo;
        info->eo = o;
        info->si = v->cursor;
        info->swapped = true;
    }

    ei = info->si;
    block_iter_skip_bytes(&ei, info->eo - info->so);
    if (block_iter_is_eof(&ei)) {
        if (info->so == info->eo) {
            return;
        }
        info->eo -= block_iter_prev_char(&ei, &u);
    }
    if (v->selection == SELECT_LINES) {
        info->so -= block_iter_bol(&info->si);
        info->eo += block_iter_eat_line(&ei);
    } else {
        // Character under cursor belongs to the selection
        info->eo += block_iter_next_column(&ei);
    }
}

size_t prepare_selection(View *v)
{
    SelectionInfo info;
    init_selection(v, &info);
    v->cursor = info.si;
    return info.eo - info.so;
}

char *view_get_selection(View *v, size_t *size)
{
    char *buf = NULL;

    *size = 0;
    if (v->selection) {
        BlockIter save = v->cursor;
        *size = prepare_selection(v);
        buf = block_iter_get_bytes(&v->cursor, *size);
        v->cursor = save;
    }
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
