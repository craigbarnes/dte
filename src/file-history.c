#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "file-history.h"
#include "common.h"
#include "error.h"
#include "util/ascii.h"
#include "util/ptr-array.h"
#include "util/strtonum.h"
#include "util/wbuf.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"

typedef struct {
    int row, col;
    size_t filename_len;
    char filename[];
} HistoryEntry;

static PointerArray history = PTR_ARRAY_INIT;

#define max_history_size 500

static bool entry_match(const HistoryEntry *e, const char *filename, size_t len)
{
    return len == e->filename_len && memcmp(filename, e->filename, len) == 0;
}

void add_file_history(int row, int col, const char *filename)
{
    const size_t filename_len = strlen(filename);

    for (size_t i = 0; i < history.count; i++) {
        HistoryEntry *e = history.ptrs[i];
        if (entry_match(e, filename, filename_len)) {
            ptr_array_remove_idx(&history, i);
            if (row > 1 || col > 1) {
                e->row = row;
                e->col = col;
                // Re-insert at end of array
                ptr_array_add(&history, e);
            } else {
                free(e);
            }
            return;
        }
    }

    if (row <= 1 && col <= 1) {
        return;
    }

    while (history.count >= max_history_size) {
        free(ptr_array_remove_idx(&history, 0));
    }

    HistoryEntry *e = xmalloc(sizeof(HistoryEntry) + filename_len);
    e->row = row;
    e->col = col;
    e->filename_len = filename_len;
    memcpy(e->filename, filename, filename_len);
    ptr_array_add(&history, e);
}

void load_file_history(const char *filename)
{
    char *buf;
    ssize_t size = read_file(filename, &buf);
    if (size < 0) {
        if (errno != ENOENT) {
            error_msg("Error reading %s: %s", filename, strerror(errno));
        }
        return;
    }

    ssize_t pos = 0;
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

    for (size_t i = 0, n = history.count; i < n; i++) {
        const HistoryEntry *e = history.ptrs[i];
        char tmp[64];
        size_t tmp_len = xsnprintf(tmp, sizeof(tmp), "%d %d ", e->row, e->col);
        wbuf_write(&buf, tmp, tmp_len);
        wbuf_write(&buf, e->filename, e->filename_len);
        wbuf_write_ch(&buf, '\n');
    }

    wbuf_flush(&buf);
    close(buf.fd);
}

bool find_file_in_history(const char *filename, int *row, int *col)
{
    const size_t filename_len = strlen(filename);
    for (size_t i = 0, n = history.count; i < n; i++) {
        const HistoryEntry *e = history.ptrs[i];
        if (entry_match(e, filename, filename_len)) {
            *row = e->row;
            *col = e->col;
            return true;
        }
    }
    return false;
}
