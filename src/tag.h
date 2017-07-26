#ifndef TAG_H
#define TAG_H

#include "ctags.h"
#include "ptr-array.h"

struct tag_file *load_tag_file(void);
void free_tags(struct ptr_array *tags);
void tag_file_find_tags(struct tag_file *tf, const char *filename, const char *name, struct ptr_array *tags);
char *tag_file_get_tag_filename(struct tag_file *tf, struct tag *t);

void collect_tags(struct tag_file *tf, const char *prefix);

#endif
