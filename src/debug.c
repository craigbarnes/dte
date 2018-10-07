#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "debug.h"
#include "editor.h"
#include "terminal/terminfo.h"
#include "util/xreadwrite.h"

void term_cleanup(void)
{
    if (editor.status == EDITOR_INITIALIZING) {
        return;
    }
    if (!editor.child_controls_terminal) {
        editor.ui_end();
    }
    const char *const deinit = terminal.control_codes.deinit;
    if (deinit) {
        xwrite(STDOUT_FILENO, deinit, strlen(deinit));
    }
}

#if DEBUG >= 1
NORETURN
void bug(const char *file, int line, const char *func, const char *fmt, ...) {
    term_cleanup();

    fprintf(stderr, "\n%s:%d: **BUG** in %s() function: '", file, line, func);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputs("'\n", stderr);

    abort(); // For core dump
}
#endif

#ifdef DEBUG_PRINT
void debug_print(const char *function, const char *fmt, ...)
{
    static int fd = -1;
    if (fd < 0) {
        char *filename = editor_file("debug.log");
        fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
        free(filename);
        BUG_ON(fd < 0);

        // Don't leak file descriptor to parent processes
        int r = fcntl(fd, F_SETFD, FD_CLOEXEC);
        DEBUG_VAR(r);
        BUG_ON(r == -1);
    }

    char buf[4096];
    size_t write_max = ARRAY_COUNT(buf);
    const int len1 = snprintf(buf, write_max, "%s: ", function);
    BUG_ON(len1 <= 0 || len1 > write_max);
    write_max -= len1;

    va_list ap;
    va_start(ap, fmt);
    const int len2 = vsnprintf(buf + len1, write_max, fmt, ap);
    va_end(ap);
    BUG_ON(len2 <= 0 || len2 > write_max);

    xwrite(fd, buf, len1 + len2);
}
#endif
