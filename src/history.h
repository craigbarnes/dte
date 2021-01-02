#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include "util/ptr-array.h"

typedef struct {
    const char *filename;
    size_t max_entries;
    PointerArray entries;
} History;

void history_add(History *history, const char *text);
bool history_search_forward(const History *history, ssize_t *pos, const char *text);
bool history_search_backward(const History *history, ssize_t *pos, const char *text);
void history_load(History *history, const char *filename);
void history_save(const History *history);

#endif
