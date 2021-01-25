#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"
#include "error.h"
#include "util/debug.h"
#include "util/line-iter.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "util/xstdio.h"

void history_add(History *history, const char *text)
{
    if (text[0] == '\0') {
        return;
    }

    HashMap *map = &history->entries;
    const HashMapEntry *map_entry = hashmap_find(map, text);
    HistoryEntry *e;

    if (map_entry) {
        e = map_entry->value;
        BUG_ON(!e);
        if (e == history->last) {
            return;
        }
        BUG_ON(!e->next);
        BUG_ON(map->count < 2);
        if (unlikely(e == history->first)) {
            HistoryEntry *new_first = e->next;
            new_first->prev = NULL;
            history->first = new_first;
        } else {
            BUG_ON(!e->prev);
            e->prev->next = e->next;
            e->next->prev = e->prev;
        }
    } else {
        e = xnew(HistoryEntry, 1);
        e->text = xstrdup(text);
        hashmap_insert(map, e->text, e);
        if (unlikely(map->count == 1)) {
            e->next = NULL;
            e->prev = NULL;
            history->first = e;
            history->last = e;
            return;
        }
        if (map->count == history->max_entries) {
            HistoryEntry *old_first = history->first;
            HistoryEntry *new_first = old_first->next;
            new_first->prev = NULL;
            history->first = new_first;
            HistoryEntry *removed = hashmap_remove(map, old_first->text);
            BUG_ON(removed != old_first);
            free(removed);
        }
    }

    HistoryEntry *old_last = history->last;
    e->next = NULL;
    e->prev = old_last;
    history->last = e;
    old_last->next = e;
}

bool history_search_forward (
    const History *history,
    const HistoryEntry **pos,
    const char *text
) {
    const HistoryEntry *start = *pos ? (*pos)->prev : history->last;
    for (const HistoryEntry *e = start; e; e = e->prev) {
        if (str_has_prefix(e->text, text)) {
            *pos = e;
            return true;
        }
    }
    return false;
}

bool history_search_backward (
    const History *history,
    const HistoryEntry **pos,
    const char *text
) {
    const HistoryEntry *start = *pos ? (*pos)->next : history->first;
    for (const HistoryEntry *e = start; e; e = e->next) {
        if (str_has_prefix(e->text, text)) {
            *pos = e;
            return true;
        }
    }
    return false;
}

void history_load(History *history, const char *filename)
{
    BUG_ON(!history);
    BUG_ON(!filename);
    BUG_ON(history->filename);
    BUG_ON(history->max_entries < 32);

    char *buf;
    const ssize_t ssize = read_file(filename, &buf);
    if (ssize < 0) {
        if (errno != ENOENT) {
            error_msg("Error reading %s: %s", filename, strerror(errno));
        }
        return;
    }

    hashmap_init(&history->entries, history->max_entries);
    const size_t size = ssize;
    size_t pos = 0;
    while (pos < size) {
        history_add(history, buf_next_line(buf, &pos, size));
    }

    free(buf);
    history->filename = filename;
}

void history_save(const History *history)
{
    const char *filename = history->filename;
    if (!filename) {
        return;
    }

    FILE *f = xfopen(filename, "w", O_CLOEXEC, 0666);
    if (!f) {
        error_msg("Error creating %s: %s", filename, strerror(errno));
        return;
    }

    for (const HistoryEntry *e = history->first; e; e = e->next) {
        fputs(e->text, f);
        fputc('\n', f);
    }

    fclose(f);
}
