#include "file-history.h"
#include "common.h"
#include "wbuf.h"
#include "ptr-array.h"
#include "error.h"

typedef struct {
    int row, col;
    char *filename;
} HistoryEntry;

static PointerArray history = PTR_ARRAY_INIT;

#define max_history_size 500

void add_file_history(int row, int col, const char *filename)
{
    HistoryEntry *e;
    for (size_t i = 0; i < history.count; i++) {
        e = history.ptrs[i];
        if (streq(filename, e->filename)) {
            e->row = row;
            e->col = col;
            // Keep newest at end of the array
            ptr_array_add(&history, ptr_array_remove_idx(&history, i));
            return;
        }
    }

    while (max_history_size && history.count >= max_history_size) {
        e = ptr_array_remove_idx(&history, 0);
        free(e->filename);
        free(e);
    }

    if (!max_history_size) {
        return;
    }

    e = xnew(HistoryEntry, 1);
    e->row = row;
    e->col = col;
    e->filename = xstrdup(filename);
    ptr_array_add(&history, e);
}

void load_file_history(const char *filename)
{
    ssize_t size, pos = 0;
    char *buf;

    size = read_file(filename, &buf);
    if (size < 0) {
        if (errno != ENOENT) {
            error_msg("Error reading %s: %s", filename, strerror(errno));
        }
        return;
    }
    while (pos < size) {
        const char *line = buf_next_line(buf, &pos, size);
        long row, col;

        if (!parse_long(&line, &row) || row <= 0) {
            continue;
        }
        while (ascii_isspace(*line)) {
            line++;
        }
        if (!parse_long(&line, &col) || col <= 0) {
            continue;
        }
        while (ascii_isspace(*line)) {
            line++;
        }
        add_file_history(row, col, line);
    }
    free(buf);
}

void save_file_history(const char *filename)
{
    WriteBuffer buf = WBUF_INIT;

    buf.fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (buf.fd < 0) {
        error_msg("Error creating %s: %s", filename, strerror(errno));
        return;
    }
    for (size_t i = 0; i < history.count; i++) {
        HistoryEntry *e = history.ptrs[i];
        char str[64];
        snprintf(str, sizeof(str), "%d %d ", e->row, e->col);
        wbuf_write_str(&buf, str);
        wbuf_write_str(&buf, e->filename);
        wbuf_write_ch(&buf, '\n');
    }
    wbuf_flush(&buf);
    close(buf.fd);
}

bool find_file_in_history(const char *filename, int *row, int *col)
{
    for (size_t i = 0; i < history.count; i++) {
        HistoryEntry *e = history.ptrs[i];
        if (streq(filename, e->filename)) {
            *row = e->row;
            *col = e->col;
            return true;
        }
    }
    return false;
}
