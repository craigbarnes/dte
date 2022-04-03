#ifndef TAG_H
#define TAG_H

#include "ctags.h"
#include "msg.h"
#include "util/ptr-array.h"

void tag_lookup(const char *name, const char *filename, MessageArray *messages);
void collect_tags(PointerArray *a, const char *prefix);
void tag_file_free(void);

#endif
