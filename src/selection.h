#ifndef SELECTION_H
#define SELECTION_H

#include "view.h"

typedef struct {
    BlockIter si;
    long so;
    long eo;
    bool swapped;
} SelectionInfo;

void init_selection(const View *v, SelectionInfo *info);
long prepare_selection(View *v);
char *view_get_selection(View *v, long *size);
int get_nr_selected_lines(const SelectionInfo *info);
int get_nr_selected_chars(const SelectionInfo *info);

#endif
