#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"
#include "error.h"
#include "util/arith.h"
#include "util/debug.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "util/xstdio.h"

void history_append(History *history, const char *text)
{
    BUG_ON(history->max_entries < 2);
    if (text[0] == '\0') {
        return;
    }

    HashMap *map = &history->entries;
    HistoryEntry *e = hashmap_get(map, text);

    if (e) {
        if (e == history->last) {
            // Existing entry already at end of list; nothing more to do
            return;
        }
        // Remove existing entry from list
        e->next->prev = e->prev;
        if (unlikely(e == history->first)) {
            history->first = e->next;
        } else {
            e->prev->next = e->next;
        }
    } else {
        if (map->count == history->max_entries) {
            // History is full; recycle oldest entry
            HistoryEntry *old_first = history->first;
            HistoryEntry *new_first = old_first->next;
            new_first->prev = NULL;
            history->first = new_first;
            e = hashmap_remove(map, old_first->text);
            BUG_ON(e != old_first);
        } else {
            e = xnew(HistoryEntry, 1);
        }
        e->text = xstrdup(text);
        hashmap_insert(map, e->text, e);
    }

    // Insert entry at end of list
    HistoryEntry *old_last = history->last;
    e->next = NULL;
    e->prev = old_last;
    history->last = e;
    if (likely(old_last)) {
        old_last->next = e;
    } else {
        history->first = e;
    }
}

bool history_search_forward (
    const History *history,
    const HistoryEntry **pos,
    const char *text
) {
    const HistoryEntry *start = *pos ? (*pos)->prev : history->last;
    size_t text_len = strlen(text);
    for (const HistoryEntry *e = start; e; e = e->prev) {
        if (str_has_strn_prefix(e->text, text, text_len)) {
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
    size_t text_len = strlen(text);
    for (const HistoryEntry *e = start; e; e = e->next) {
        if (str_has_strn_prefix(e->text, text, text_len)) {
            *pos = e;
            return true;
        }
    }
    return false;
}

void history_load(History *history, char *filename, size_t size_limit)
{
    BUG_ON(history->filename);
    BUG_ON(history->max_entries < 2);
    hashmap_init(&history->entries, history->max_entries);
    history->filename = filename;

    char *buf;
    const ssize_t ssize = read_file(filename, &buf, size_limit);
    if (ssize < 0) {
        if (errno != ENOENT) {
            error_msg_("Error reading %s: %s", filename, strerror(errno));
        }
        return;
    }

    for (size_t pos = 0, size = ssize; pos < size; ) {
        history_append(history, buf_next_line(buf, &pos, size));
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
        error_msg_("Error creating %s: %s", filename, strerror(errno));
        return;
    }

    for (const HistoryEntry *e = history->first; e; e = e->next) {
        xfputs(e->text, f);
        xfputc('\n', f);
    }

    fclose(f);
}

void history_free(History *history)
{
    hashmap_free(&history->entries, free);
    free(history->filename);
    history->filename = NULL;
    history->first = NULL;
    history->last = NULL;
}

String history_dump(const History *history)
{
    const size_t nr_entries = history->entries.count;
    const size_t size = next_multiple(16 * nr_entries, 4096);
    String buf = string_new(size);
    size_t n = 0;
    for (const HistoryEntry *e = history->first; e; e = e->next, n++) {
        string_append_cstring(&buf, e->text);
        string_append_byte(&buf, '\n');
    }
    BUG_ON(n != nr_entries);
    return buf;
}
