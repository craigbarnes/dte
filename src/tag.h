#ifndef TAG_H
#define TAG_H

#include "ctags.h"
#include "util/ptr-array.h"

TagFile *load_tag_file(void);
void free_tags(PointerArray *tags);
char *tag_file_get_tag_filename(const TagFile *tf, const Tag *t);
void collect_tags(const TagFile *tf, const char *prefix);

void tag_file_find_tags (
    const TagFile *tf,
    const char *filename,
    const char *name,
    PointerArray *tags
);

#endif
