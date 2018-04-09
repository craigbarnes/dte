#ifndef GIT_OPEN_H
#define GIT_OPEN_H

#include "ptr-array.h"
#include "term.h"

typedef struct {
    PointerArray files;
    char *all_files;
    size_t size;
    size_t selected;
    size_t scroll;
} GitOpenState;

extern GitOpenState git_open;

void git_open_reload(void);
void git_open_keypress(Key key);

#endif
