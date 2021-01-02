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
    PointerArray *entries = &history->entries;
    for (size_t i = 0, n = entries->count; i < n; i++) {
        if (streq(entries->ptrs[i], text)) {
            // Move identical entry to end
            ptr_array_append(entries, ptr_array_remove_idx(entries, i));
            return;
        }
    }
    if (entries->count == history->max_entries) {
        free(ptr_array_remove_idx(entries, 0));
    }
    ptr_array_append(entries, xstrdup(text));
}

bool history_search_forward (
    const History *history,
    ssize_t *pos,
    const char *text
) {
    ssize_t i = *pos;
    while (--i >= 0) {
        if (str_has_prefix(history->entries.ptrs[i], text)) {
            *pos = i;
            return true;
        }
    }
    return false;
}

bool history_search_backward (
    const History *history,
    ssize_t *pos,
    const char *text
) {
    ssize_t i = *pos;
    while (++i < history->entries.count) {
        if (str_has_prefix(history->entries.ptrs[i], text)) {
            *pos = i;
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
    BUG_ON(history->entries.count > 0);
    BUG_ON(history->entries.alloc > 0);
    BUG_ON(history->entries.ptrs);

    char *buf;
    const ssize_t ssize = read_file(filename, &buf);
    if (ssize < 0) {
        if (errno != ENOENT) {
            error_msg("Error reading %s: %s", filename, strerror(errno));
        }
        return;
    }

    ptr_array_init(&history->entries, history->max_entries);
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

    for (size_t i = 0, n = history->entries.count; i < n; i++) {
        fputs(history->entries.ptrs[i], f);
        fputc('\n', f);
    }

    fclose(f);
}
