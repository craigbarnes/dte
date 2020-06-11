#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "history.h"
#include "error.h"
#include "util/line-iter.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

// Add item to end of array
void history_add(PointerArray *history, const char *text, size_t max_entries)
{
    if (text[0] == '\0') {
        return;
    }

    // Don't add identical entries
    for (size_t i = 0, n = history->count; i < n; i++) {
        if (streq(history->ptrs[i], text)) {
            // Move identical entry to end
            ptr_array_append(history, ptr_array_remove_idx(history, i));
            return;
        }
    }
    if (history->count == max_entries) {
        free(ptr_array_remove_idx(history, 0));
    }
    ptr_array_append(history, xstrdup(text));
}

bool history_search_forward (
    const PointerArray *history,
    ssize_t *pos,
    const char *text
) {
    ssize_t i = *pos;
    while (--i >= 0) {
        if (str_has_prefix(history->ptrs[i], text)) {
            *pos = i;
            return true;
        }
    }
    return false;
}

bool history_search_backward (
    const PointerArray *history,
    ssize_t *pos,
    const char *text
) {
    ssize_t i = *pos;
    while (++i < history->count) {
        if (str_has_prefix(history->ptrs[i], text)) {
            *pos = i;
            return true;
        }
    }
    return false;
}

void history_load(PointerArray *history, const char *filename, size_t max_entries)
{
    char *buf;
    const ssize_t ssize = read_file(filename, &buf);
    if (ssize < 0) {
        if (errno != ENOENT) {
            error_msg("Error reading %s: %s", filename, strerror(errno));
        }
        return;
    }

    const size_t size = ssize;
    size_t pos = 0;
    while (pos < size) {
        history_add(history, buf_next_line(buf, &pos, size), max_entries);
    }

    free(buf);
}

// Like fopen(), but also sets the file mode
FILE *history_fopen(const char *filename)
{
    int fd = xopen(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd < 0) {
        goto error;
    }
    FILE *file = fdopen(fd, "w");
    if (!file) {
        xclose(fd);
        goto error;
    }
    return file;
error:
    error_msg("Error creating %s: %s", filename, strerror(errno));
    return NULL;
}

void history_save(const PointerArray *history, const char *filename)
{
    FILE *f = history_fopen(filename);
    if (!f) {
        return;
    }
    for (size_t i = 0, n = history->count; i < n; i++) {
        fputs(history->ptrs[i], f);
        fputc('\n', f);
    }
    fclose(f);
}
