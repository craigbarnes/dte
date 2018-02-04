#include "history.h"
#include "common.h"
#include "wbuf.h"
#include "ptr-array.h"
#include "error.h"

// Add item to end of array
void history_add(PointerArray *history, const char *text, size_t max_entries)
{
    if (text[0] == 0) {
        return;
    }

    // Don't add identical entries
    for (size_t i = 0; i < history->count; i++) {
        if (streq(history->ptrs[i], text)) {
            // Move identical entry to end
            ptr_array_add(history, ptr_array_remove_idx(history, i));
            return;
        }
    }
    if (history->count == max_entries) {
        free(ptr_array_remove_idx(history, 0));
    }
    ptr_array_add(history, xstrdup(text));
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
    ssize_t size, pos = 0;

    size = read_file(filename, &buf);
    if (size < 0) {
        if (errno != ENOENT) {
            error_msg("Error reading %s: %s", filename, strerror(errno));
        }
        return;
    }
    while (pos < size) {
        history_add(history, buf_next_line(buf, &pos, size), max_entries);
    }
    free(buf);
}

void history_save(PointerArray *history, const char *filename)
{
    WriteBuffer buf = WBUF_INIT;

    buf.fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (buf.fd < 0) {
        error_msg("Error creating %s: %s", filename, strerror(errno));
        return;
    }

    for (size_t i = 0; i < history->count; i++) {
        wbuf_write_str(&buf, history->ptrs[i]);
        wbuf_write_ch(&buf, '\n');
    }
    wbuf_flush(&buf);
    close(buf.fd);
}
