#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "exitcode.h"
#include "xreadwrite.h"
#include "xsnprintf.h"

static CleanupHandler cleanup_handler = NULL;
static void *cleanup_userdata = NULL;

void set_fatal_error_cleanup_handler(CleanupHandler handler, void *userdata)
{
    cleanup_handler = handler;
    cleanup_userdata = userdata;
}

static void cleanup(void)
{
    if (cleanup_handler) {
        cleanup_handler(cleanup_userdata);
    }
}

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
    cleanup();
}
#endif

static void print_stack_trace(void)
{
    #ifdef ASAN_ENABLED
        fputs("\nStack trace:\n", stderr);
        __sanitizer_print_stack_trace();
    #endif
}

#if DEBUG >= 1
noreturn
void bug(const char *file, int line, const char *func, const char *fmt, ...)
{
    char str[512];
    va_list ap;
    str[0] = '\0';
    va_start(ap, fmt);
    vsnprintf(str, sizeof str, fmt, ap);
    va_end(ap);

    debug_log(file, line, "BUG in %s(): '%s'", func, str);
    cleanup();

    fprintf(stderr, "\n%s:%d: **BUG** in %s(): '%s'\n", file, line, func, str);
    print_stack_trace();
    fflush(stderr);
    abort();
}
#endif

// Error handler for unrecoverable system errors during runtime
noreturn void fatal_error(const char *msg, int err)
{
    DEBUG_LOG("%s: %s", msg, strerror(err));
    cleanup();
    errno = err;
    perror(msg);
    print_stack_trace();
    abort();
}
