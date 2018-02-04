#ifndef TAG_H
#define TAG_H

#include "ctags.h"
#include "ptr-array.h"

TagFile *load_tag_file(void);
void free_tags(PointerArray *tags);
void tag_file_find_tags(TagFile *tf, const char *filename, const char *name, PointerArray *tags);
char *tag_file_get_tag_filename(TagFile *tf, Tag *t);

void collect_tags(TagFile *tf, const char *prefix);

#endif
