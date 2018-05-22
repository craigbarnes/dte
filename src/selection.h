#ifndef SELECTION_H
#define SELECTION_H

#include "view.h"

typedef struct {
    BlockIter si;
    size_t so;
    size_t eo;
    bool swapped;
} SelectionInfo;

void init_selection(const View *v, SelectionInfo *info);
size_t prepare_selection(View *v);
char *view_get_selection(View *v, size_t *size);
size_t get_nr_selected_lines(const SelectionInfo *info);
size_t get_nr_selected_chars(const SelectionInfo *info);

#endif
