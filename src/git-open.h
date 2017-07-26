#ifndef GIT_OPEN_H
#define GIT_OPEN_H

#include "ptr-array.h"
#include "term.h"

struct git_open {
	struct ptr_array files;
	char *all_files;
	long size;
	int selected;
	int scroll;
};

extern struct git_open git_open;

void git_open_reload(void);
void git_open_keypress(int key);

#endif
