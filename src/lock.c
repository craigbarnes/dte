#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "lock.h"
#include "error.h"
#include "util/debug.h"
#include "util/log.h"
#include "util/path.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"
#include "util/xsnprintf.h"

// These are initialized during early startup and then never changed,
// so they're deemed an "acceptable" use of globals:
static const char *file_locks;
static const char *file_locks_lock;
static mode_t file_locks_mode = 0666;
static pid_t editor_pid;

void init_file_locks_context(const char *fallback_dir, pid_t pid)
{
    BUG_ON(file_locks);
    const char *dir = xgetenv("XDG_RUNTIME_DIR");
    if (!dir) {
        LOG_INFO("$XDG_RUNTIME_DIR not set");
        dir = fallback_dir;
    } else if (unlikely(!path_is_absolute(dir))) {
        LOG_WARNING("$XDG_RUNTIME_DIR invalid (not an absolute path)");
        dir = fallback_dir;
    } else {
        // Set sticky bit (see XDG Base Directory Specification)
        #ifdef S_ISVTX
            file_locks_mode |= S_ISVTX;
        #endif
    }

    file_locks = path_join(dir, "dte-locks");
    file_locks_lock = path_join(dir, "dte-locks.lock");
    editor_pid = pid;
    LOG_INFO("locks file: %s", file_locks);
}

static bool process_exists(pid_t pid)
{
    return !kill(pid, 0);
}

static pid_t rewrite_lock_file(char *buf, size_t *sizep, const char *filename)
{
    const size_t filename_len = strlen(filename);
    size_t size = *sizep;
    pid_t other_pid = 0;

    for (size_t pos = 0, bol = 0; pos < size; bol = pos) {
        StringView line = buf_slice_next_line(buf, &pos, size);
        uintmax_t num;
        size_t numlen = buf_parse_uintmax(line.data, line.length, &num);
        if (unlikely(numlen == 0 || num != (pid_t)num)) {
            goto remove_line;
        }

        strview_remove_prefix(&line, numlen);
        if (unlikely(!strview_has_prefix(&line, " /"))) {
            goto remove_line;
        }
        strview_remove_prefix(&line, 1);

        bool same = strview_equal_strn(&line, filename, filename_len);
        pid_t pid = (pid_t)num;
        if (pid == editor_pid) {
            if (same) {
                goto remove_line;
            }
            continue;
        } else if (process_exists(pid)) {
            if (same) {
                other_pid = pid;
            }
            continue;
        }

        remove_line:
        memmove(buf + bol, buf + pos, size - pos);
        size -= pos - bol;
        pos = bol;
    }

    *sizep = size;
    return other_pid;
}

static bool lock_or_unlock(const char *filename, bool lock)
{
    BUG_ON(!file_locks);
    if (streq(filename, file_locks) || streq(filename, file_locks_lock)) {
        return true;
    }

    mode_t mode = file_locks_mode;
    int tries = 0;
    int wfd;
    while (1) {
        wfd = xopen(file_locks_lock, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, mode);
        if (wfd >= 0) {
            break;
        }

        if (errno != EEXIST) {
            return error_msg("Error creating %s: %s", file_locks_lock, strerror(errno));
        }
        if (++tries == 3) {
            if (unlink(file_locks_lock)) {
                return error_msg (
                    "Error removing stale lock file %s: %s",
                    file_locks_lock,
                    strerror(errno)
                );
            }
            error_msg("Stale lock file %s removed", file_locks_lock);
        } else {
            const struct timespec req = {
                .tv_sec = 0,
                .tv_nsec = 100 * 1000000,
            };
            nanosleep(&req, NULL);
        }
    }

    char *buf = NULL;
    ssize_t ssize = read_file(file_locks, &buf);
    if (ssize < 0) {
        if (errno != ENOENT) {
            error_msg("Error reading %s: %s", file_locks, strerror(errno));
            goto error;
        }
        ssize = 0;
    }

    size_t size = (size_t)ssize;
    pid_t pid = rewrite_lock_file(buf, &size, filename);
    if (lock) {
        if (pid == 0) {
            intmax_t p = (intmax_t)editor_pid;
            size_t n = strlen(filename) + DECIMAL_STR_MAX(pid) + 4;
            xrenew(buf, size + n);
            size += xsnprintf(buf + size, n, "%jd %s\n", p, filename);
        } else {
            intmax_t p = (intmax_t)pid;
            error_msg("File is locked (%s) by process %jd", file_locks, p);
        }
    }

    if (xwrite_all(wfd, buf, size) < 0) {
        error_msg("Error writing %s: %s", file_locks_lock, strerror(errno));
        goto error;
    }

    int r = xclose(wfd);
    wfd = -1;
    if (r != 0) {
        error_msg("Error closing %s: %s", file_locks_lock, strerror(errno));
        goto error;
    }

    if (rename(file_locks_lock, file_locks)) {
        const char *err = strerror(errno);
        error_msg("Renaming %s to %s: %s", file_locks_lock, file_locks, err);
        goto error;
    }

    free(buf);
    return (pid == 0);

error:
    unlink(file_locks_lock);
    free(buf);
    if (wfd >= 0) {
        xclose(wfd);
    }
    return false;
}

bool lock_file(const char *filename)
{
    return lock_or_unlock(filename, true);
}

void unlock_file(const char *filename)
{
    lock_or_unlock(filename, false);
}
