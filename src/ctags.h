#ifndef CTAGS_H
#define CTAGS_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

typedef struct {
    char *filename;
    char *buf;
    long size;
    time_t mtime;
} TagFile;

typedef struct {
    char *name;
    char *filename;
    char *pattern;
    char *member;
    char *typeref;
    int line;
    char kind;
    bool local;
} Tag;

bool next_tag(TagFile *tf, size_t *posp, const char *prefix, int exact, Tag *t);
void free_tag(Tag *t);

#endif
