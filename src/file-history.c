#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "file-history.h"
#include "error.h"
#include "util/path.h"
#include "util/ptr-array.h"
#include "util/readfile.h"
#include "util/strtonum.h"
#include "util/wbuf.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"

typedef struct {
    unsigned long row, col;
    size_t filename_len;
    char filename[];
} HistoryEntry;

static PointerArray history = PTR_ARRAY_INIT;

#define max_history_size 500

static bool entry_match(const HistoryEntry *e, const char *filename, size_t len)
{
    return len == e->filename_len && memcmp(filename, e->filename, len) == 0;
}

static ssize_t lookup_entry_index(const char *filename, size_t filename_len)
{
    for (size_t i = 0, n = history.count; i < n; i++) {
        const HistoryEntry *e = history.ptrs[i];
        if (entry_match(e, filename, filename_len)) {
            return i;
        }
    }
    return -1;
}

void add_file_history(unsigned long row, unsigned long col, const char *filename)
{
    const size_t filename_len = strlen(filename);
    const ssize_t idx = lookup_entry_index(filename, filename_len);
    if (idx >= 0) {
        HistoryEntry *e = history.ptrs[idx];
        ptr_array_remove_idx(&history, (size_t)idx);
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

static bool parse_ulong(const char **strp, unsigned long *valp)
{
    const char *str = *strp;
    const size_t len = strlen(str);
    const size_t ndigits = buf_parse_ulong(str, len, valp);
    if (ndigits > 0) {
        *strp = str + ndigits;
        return true;
    }
    return false;
}

void load_file_history(const char *filename)
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
        const char *line = buf_next_line(buf, &pos, size);
        unsigned long row, col;
        if (!parse_ulong(&line, &row) || row == 0 || *line++ != ' ') {
            continue;
        }
        if (!parse_ulong(&line, &col) || col == 0 || *line++ != ' ') {
            continue;
        }
        const char *path = line;
        if (!path_is_absolute(path)) {
            continue;
        }
        add_file_history(row, col, path);
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
        wbuf_need_space(&buf, 64);
        buf.fill += xsnprintf(buf.buf + buf.fill, 64, "%lu %lu ", e->row, e->col);
        wbuf_write(&buf, e->filename, e->filename_len);
        wbuf_write_ch(&buf, '\n');
    }

    wbuf_flush(&buf);
    close(buf.fd);
}

bool find_file_in_history(const char *filename, unsigned long *row, unsigned long *col)
{
    const ssize_t idx = lookup_entry_index(filename, strlen(filename));
    if (idx >= 0) {
        const HistoryEntry *e = history.ptrs[idx];
        *row = e->row;
        *col = e->col;
        return true;
    }
    return false;
}
