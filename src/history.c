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
    BUG_ON(history->max_entries < 2);
    if (text[0] == '\0') {
        return;
    }

    HashMap *map = &history->entries;
    const HashMapEntry *map_entry = hashmap_find(map, text);
    HistoryEntry *e;

    if (map_entry) {
        // Existing entry found
        e = map_entry->value;
        BUG_ON(!e);
        if (e == history->last) {
            // Entry is already at the end of the list; nothing more to do
            return;
        }
        BUG_ON(!e->next);
        BUG_ON(map->count < 2);
        if (unlikely(e == history->first)) {
            // Entry is at the start of the list, remove it
            HistoryEntry *new_first = e->next;
            new_first->prev = NULL;
            history->first = new_first;
        } else {
            // Entry is in the middle of the list, remove it
            BUG_ON(!e->prev);
            e->prev->next = e->next;
            e->next->prev = e->prev;
        }
    } else {
        if (map->count == history->max_entries) {
            // Adding a new entry to a full history; remove the oldest
            // entry to make space
            HistoryEntry *old_first = history->first;
            HistoryEntry *new_first = old_first->next;
            new_first->prev = NULL;
            history->first = new_first;
            HistoryEntry *removed = hashmap_remove(map, old_first->text);
            BUG_ON(removed != old_first);
            // Instead of freeing the removed entry, just recycle the
            // allocation for the new entry
            e = removed;
            e->text = xstrdup(text);
            hashmap_insert(map, e->text, e);
        } else {
            // Adding a new entry to non-full history
            e = xnew(HistoryEntry, 1);
            e->text = xstrdup(text);
            hashmap_insert(map, e->text, e);
            if (unlikely(map->count == 1)) {
                // Special case; history is completely empty, so the new entry
                // is both the first and last entry with no prev or next
                e->next = NULL;
                e->prev = NULL;
                history->first = e;
                history->last = e;
                return;
            }
        }
    }

    // Insert the entry at the end of the list
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
    BUG_ON(history->max_entries < 2);

    hashmap_init(&history->entries, history->max_entries);
    history->filename = filename;

    char *buf;
    const ssize_t ssize = read_file(filename, &buf);
    if (ssize < 0) {
        if (errno != ENOENT) {
            error_msg("Error reading %s: %s", filename, strerror(errno));
        }
        return;
    }

    for (size_t pos = 0, size = ssize; pos < size; ) {
        history_add(history, buf_next_line(buf, &pos, size));
    }

    free(buf);
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
