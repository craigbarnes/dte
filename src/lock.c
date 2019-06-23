#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "lock.h"
#include "buffer.h"
#include "editor.h"
#include "error.h"
#include "util/ascii.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"
#include "util/xsnprintf.h"

static char *file_locks;
static char *file_locks_lock;

static bool process_exists(pid_t pid)
{
    return !kill(pid, 0);
}

static pid_t rewrite_lock_file(char *buf, ssize_t *sizep, const char *filename)
{
    size_t filename_len = strlen(filename);
    pid_t my_pid = getpid();
    ssize_t size = *sizep;
    ssize_t pos = 0;
    pid_t other_pid = 0;

    while (pos < size) {
        ssize_t bol = pos;
        bool remove_line = false;
        pid_t pid = 0;

        while (pos < size && ascii_isdigit(buf[pos])) {
            pid *= 10;
            pid += buf[pos++] - '0';
        }
        while (pos < size && (buf[pos] == ' ' || buf[pos] == '\t')) {
            pos++;
        }
        char *nl = memchr(buf + pos, '\n', size - pos);
        ssize_t next_bol = nl - buf + 1;

        bool same =
            filename_len == next_bol - 1 - pos
            && !memcmp(buf + pos, filename, filename_len)
        ;
        if (pid == my_pid) {
            if (same) {
                // lock = 1 => pid conflict. lock must be stale
                // lock = 0 => normal unlock case
                remove_line = true;
            }
        } else if (process_exists(pid)) {
            if (same) {
                other_pid = pid;
            }
        } else {
            // Release lock from dead process
            remove_line = true;
        }

        if (remove_line) {
            memmove(buf + bol, buf + next_bol, size - next_bol);
            size -= next_bol - bol;
            pos = bol;
        } else {
            pos = next_bol;
        }
    }
    *sizep = size;
    return other_pid;
}

static int lock_or_unlock(const char *filename, bool lock)
{
    if (!file_locks) {
        file_locks = editor_file("file-locks");
        file_locks_lock = editor_file("file-locks.lock");
    }

    if (streq(filename, file_locks) || streq(filename, file_locks_lock)) {
        return 0;
    }

    int tries = 0;
    int wfd;
    while (1) {
        wfd = open(file_locks_lock, O_WRONLY | O_CREAT | O_EXCL, 0666);
        if (wfd >= 0) {
            break;
        }

        if (errno != EEXIST) {
            error_msg (
                "Error creating %s: %s",
                file_locks_lock,
                strerror(errno)
            );
            return -1;
        }
        if (++tries == 3) {
            if (unlink(file_locks_lock)) {
                error_msg (
                    "Error removing stale lock file %s: %s",
                    file_locks_lock,
                    strerror(errno)
                );
                return -1;
            }
            error_msg("Stale lock file %s removed.", file_locks_lock);
        } else {
            const struct timespec req = {
                .tv_sec = 0,
                .tv_nsec = 100 * 1000000,
            };
            nanosleep(&req, NULL);
        }
    }

    char *buf = NULL;
    ssize_t size = read_file(file_locks, &buf);
    if (size < 0) {
        if (errno != ENOENT) {
            error_msg("Error reading %s: %s", file_locks, strerror(errno));
            goto error;
        }
        size = 0;
    }
    if (size > 0 && buf[size - 1] != '\n') {
        buf[size++] = '\n';
    }
    pid_t pid = rewrite_lock_file(buf, &size, filename);
    if (lock) {
        if (pid == 0) {
            const size_t n = strlen(filename) + 32;
            xrenew(buf, size + n);
            xsnprintf(buf + size, n, "%d %s\n", getpid(), filename);
            size += strlen(buf + size);
        } else {
            error_msg("File is locked (%s) by process %d", file_locks, pid);
        }
    }
    if (xwrite(wfd, buf, size) < 0) {
        error_msg("Error writing %s: %s", file_locks_lock, strerror(errno));
        goto error;
    }
    if (close(wfd)) {
        error_msg("Error closing %s: %s", file_locks_lock, strerror(errno));
        goto error;
    }
    if (rename(file_locks_lock, file_locks)) {
        error_msg (
            "Renaming %s to %s: %s",
            file_locks_lock,
            file_locks,
            strerror(errno)
        );
        goto error;
    }
    free(buf);
    return pid == 0 ? 0 : -1;
error:
    unlink(file_locks_lock);
    free(buf);
    close(wfd);
    return -1;
}

int lock_file(const char *filename)
{
    return lock_or_unlock(filename, true);
}

void unlock_file(const char *filename)
{
    lock_or_unlock(filename, false);
}
