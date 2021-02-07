#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "exitcode.h"
#include "xreadwrite.h"
#include "xsnprintf.h"

static void no_op(void) {}
static void (*cleanup_handler)(void) = no_op;

#ifdef ASAN_ENABLED
#include <sanitizer/asan_interface.h>

const char *__asan_default_options(void)
{
    return "detect_leaks=1:detect_stack_use_after_return=1";
}

void __asan_on_error(void)
{
    // This function is called when ASan detects an error. Unlike
    // callbacks set with __sanitizer_set_death_callback(), it runs
    // before the error report is printed and so allows us to clean
    // up the terminal state and avoid clobbering the stderr output.
    cleanup_handler();
}
#endif

static void print_stack_trace(void)
{
    #ifdef ASAN_ENABLED
        fputs("\nStack trace:\n", stderr);
        __sanitizer_print_stack_trace();
    #endif
}

void set_fatal_error_cleanup_handler(void (*handler)(void))
{
    cleanup_handler = handler;
}

#if DEBUG >= 1
noreturn
void bug(const char *file, int line, const char *func, const char *fmt, ...)
{
    cleanup_handler();
    fprintf(stderr, "\n%s:%d: **BUG** in %s() function: '", file, line, func);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputs("'\n", stderr);
    print_stack_trace();
    fflush(stderr);
    abort();
}
#endif

#if DEBUG >= 2
#include <sys/utsname.h>
#include <unistd.h> // isatty

static const char *dim = "";
static const char *sgr0 = "";
static int logfd = -1;

void log_init(const char *varname)
{
    BUG_ON(logfd != -1);
    const char *path = getenv(varname);
    if (!path || path[0] == '\0') {
        return;
    }

    logfd = xopen(path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0666);
    if (logfd < 0) {
        const char *err = strerror(errno);
        fprintf(stderr, "Failed to open '%s' ($%s): %s\n", path, varname, err);
        exit(EX_IOERR);
    }
    if (xwrite(logfd, "\n", 1) != 1) {
        fprintf(stderr, "Failed to write to log: %s\n", strerror(errno));
        exit(EX_IOERR);
    }

    if (isatty(logfd)) {
        dim = "\033[2m";
        sgr0 = "\033[0m";
    }

    struct utsname u;
    if (uname(&u) >= 0) {
        DEBUG_LOG("system: %s/%s %s", u.sysname, u.machine, u.release);
    } else {
        DEBUG_LOG("uname() failed: %s", strerror(errno));
    }
}

VPRINTF(3)
static void debug_logv(const char *file, int line, const char *fmt, va_list ap)
{
    if (logfd < 0) {
        return;
    }
    char buf[4096];
    size_t write_max = ARRAY_COUNT(buf) - 1;
    const size_t len1 = xsnprintf(buf, write_max, "%s%s:%d:%s ", dim, file, line, sgr0);
    write_max -= len1;
    const size_t len2 = xvsnprintf(buf + len1, write_max, fmt, ap);
    size_t n = len1 + len2;
    buf[n++] = '\n';
    xwrite(logfd, buf, n);
}

void debug_log(const char *file, int line, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    debug_logv(file, line, fmt, ap);
    va_end(ap);
}
#endif

// Error handler for unrecoverable system errors during runtime
// (e.g. ENOMEM)
noreturn void fatal_error(const char *msg, int err)
{
    DEBUG_LOG("%s: %s", msg, strerror(err));
    cleanup_handler();
    errno = err;
    perror(msg);
    print_stack_trace();
    abort();
}

// Error handler for problems encountered during initialization
// (e.g. unsupported $TERM)
noreturn void init_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

#if DEBUG >= 2
    va_list ap2;
    va_copy(ap2, ap);
    debug_logv(__FILE__, __LINE__, fmt, ap2);
    va_end(ap2);
#endif

    vfprintf(stderr, fmt, ap);
    va_end(ap);
    putc('\n', stderr);
    fflush(stderr);

    exit(1);
}
