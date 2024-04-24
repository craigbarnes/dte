#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "file-history.h"
#include "error.h"
#include "util/debug.h"
#include "util/path.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xstdio.h"

enum {
    MAX_ENTRIES = 512
};

void file_history_append(FileHistory *history, unsigned long row, unsigned long col, const char *filename)
{
    BUG_ON(row == 0);
    BUG_ON(col == 0);
    HashMap *map = &history->entries;
    FileHistoryEntry *e = hashmap_get(map, filename);

    if (e) {
        if (e == history->last) {
            e->row = row;
            e->col = col;
            return;
        }
        e->next->prev = e->prev;
        if (unlikely(e == history->first)) {
            history->first = e->next;
        } else {
            e->prev->next = e->next;
        }
    } else {
        if (map->count == MAX_ENTRIES) {
            // History is full; recycle the oldest entry
            FileHistoryEntry *old_first = history->first;
            FileHistoryEntry *new_first = old_first->next;
            new_first->prev = NULL;
            history->first = new_first;
            e = hashmap_remove(map, old_first->filename);
            BUG_ON(e != old_first);
        } else {
            e = xnew(FileHistoryEntry, 1);
        }
        e->filename = xstrdup(filename);
        hashmap_insert(map, e->filename, e);
    }

    // Insert the entry at the end of the list
    FileHistoryEntry *old_last = history->last;
    e->next = NULL;
    e->prev = old_last;
    e->row = row;
    e->col = col;
    history->last = e;
    if (likely(old_last)) {
        old_last->next = e;
    } else {
        history->first = e;
    }
}

static bool parse_ulong_field(StringView *sv, unsigned long *valp)
{
    size_t n = buf_parse_ulong(sv->data, sv->length, valp);
    if (n == 0 || *valp == 0 || sv->data[n] != ' ') {
        return false;
    }
    strview_remove_prefix(sv, n + 1);
    return true;
}

void file_history_load(FileHistory *history, char *filename, size_t size_limit)
{
    BUG_ON(!history);
    BUG_ON(!filename);
    BUG_ON(history->filename);

    hashmap_init(&history->entries, MAX_ENTRIES);
    history->filename = filename;

    char *buf;
    const ssize_t ssize = read_file(filename, &buf, size_limit);
    if (ssize < 0) {
        if (errno != ENOENT) {
            error_msg("Error reading %s: %s", filename, strerror(errno));
        }
        return;
    }

    for (size_t pos = 0, size = ssize; pos < size; ) {
        unsigned long row, col;
        StringView line = buf_slice_next_line(buf, &pos, size);
        if (unlikely(
            !parse_ulong_field(&line, &row)
            || !parse_ulong_field(&line, &col)
            || line.length < 2
            || line.data[0] != '/'
            || buf[pos - 1] != '\n'
        )) {
            continue;
        }
        buf[pos - 1] = '\0'; // null-terminate line, by replacing '\n' with '\0'
        file_history_append(history, row, col, line.data);
    }

    free(buf);
}

void file_history_save(const FileHistory *history)
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

    for (const FileHistoryEntry *e = history->first; e; e = e->next) {
        xfprintf(f, "%lu %lu %s\n", e->row, e->col, e->filename);
    }

    fclose(f);
}

bool file_history_find(const FileHistory *history, const char *filename, unsigned long *row, unsigned long *col)
{
    const FileHistoryEntry *e = hashmap_get(&history->entries, filename);
    if (!e) {
        return false;
    }
    *row = e->row;
    *col = e->col;
    return true;
}

void file_history_free(FileHistory *history)
{
    hashmap_free(&history->entries, free);
    free(history->filename);
    history->filename = NULL;
    history->first = NULL;
    history->last = NULL;
}

String file_history_dump(const FileHistory *history)
{
    size_t nr_entries = history->entries.count;
    size_t size = round_size_to_next_multiple(64 * nr_entries, 4096);
    String buf = string_new(size);
    size_t n = 0;

    for (const FileHistoryEntry *e = history->first; e; e = e->next, n++) {
        string_append_cstring(&buf, e->filename);
        string_append_byte(&buf, '\n');
    }

    BUG_ON(n != nr_entries);
    return buf;
}

String file_history_dump_relative(const FileHistory *history)
{
    char cwdbuf[8192];
    const char *cwd = getcwd(cwdbuf, sizeof(cwdbuf));
    if (unlikely(!cwd)) {
        return file_history_dump(history);
    }

    size_t nr_entries = history->entries.count;
    size_t size = round_size_to_next_multiple(16 * nr_entries, 4096);
    String buf = string_new(size);
    size_t n = 0;

    for (const FileHistoryEntry *e = history->first; e; e = e->next, n++) {
        const char *relative = path_slice_relative(e->filename, cwd);
        string_append_cstring(&buf, relative);
        string_append_byte(&buf, '\n');
    }

    BUG_ON(n != nr_entries);
    return buf;
}
