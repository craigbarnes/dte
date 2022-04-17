#ifndef CTAGS_H
#define CTAGS_H

#include <stdbool.h>
#include "util/string-view.h"

typedef struct {
    StringView name; // Name of tag (points into TagFile::buf)
    StringView filename; // File containing tag (points into TagFile::buf)
    char *pattern; // Regex pattern used to locate tag (escaped ex command)
    unsigned long lineno; // Line number in file (mutually exclusive with pattern)
    char kind; // ASCII letter representing type of tag (e.g. f=function)
    bool local; // Indicates if tag is local to file (e.g. "static" in C)
} Tag;

bool next_tag (
    const char *buf,
    size_t buf_len,
    size_t *posp,
    const char *prefix,
    bool exact,
    Tag *t
);

void free_tag(Tag *t);

#endif
