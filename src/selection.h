#ifndef SELECTION_H
#define SELECTION_H

#include "view.h"

struct selection_info {
	struct block_iter si;
	long so;
	long eo;
	bool swapped;
};

void init_selection(struct view *v, struct selection_info *info);
long prepare_selection(struct view *v);
char *view_get_selection(struct view *v, long *size);
int get_nr_selected_lines(struct selection_info *info);
int get_nr_selected_chars(struct selection_info *info);

#endif
