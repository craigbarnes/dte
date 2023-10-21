#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "log.h"

static CleanupHandler cleanup_handler = NULL;
static void *cleanup_userdata = NULL;

void set_fatal_error_cleanup_handler(CleanupHandler handler, void *userdata)
{
    cleanup_handler = handler;
    cleanup_userdata = userdata;
}

void fatal_error_cleanup(void)
{
    if (cleanup_handler) {
        cleanup_handler(cleanup_userdata);
    }
}

static void cleanup(void)
{
    // Blocking signals here prevents the following call chain from leaving
    // the above globals in an undefined state for signal handlers:
    //
    //  cleanup() -> cleanup_handler() -> set_fatal_error_cleanup_handler()
    //
    // Without this, one of the other functions below might call cleanup()
    // and then get interrupted in the middle of a sequence of instructions
    // that aren't async-signal-safe (see: signal-safety(7)).
    sigset_t mask, oldmask;
    sigfillset(&mask);
    sigprocmask(SIG_SETMASK, &mask, &oldmask);
    fatal_error_cleanup();
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
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
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    const char *str = likely(r >= 0) ? buf : "??";
    log_msg(LOG_LEVEL_CRITICAL, file, line, "BUG in %s(): '%s'", func, str);
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
    LOG_CRITICAL("%s: %s", msg, strerror(err));
    cleanup();
    errno = err;
    perror(msg);
    print_stack_trace();
    abort();
}
