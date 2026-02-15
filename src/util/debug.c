#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "debug.h"
#include "log.h"
#include "terminal/mode.h"
#include "xreadwrite.h"

// Set to 1 when the terminal needs to be reset before terminating abnormally,
// e.g. via bug(), fatal_error(), handle_fatal_signal(), etc.
volatile sig_atomic_t need_term_reset_on_fatal_error = 0; // NOLINT(*non-const-global*)

// This serves a similar purpose to ui_end(), but using only minimal
// program state, so as to be suitable for use in handle_fatal_signal()
// and as an AddressSanitizer callback
static void cleanup(void)
{
    if (!need_term_reset_on_fatal_error) {
        return;
    }

    static const char reset[] = "\033c"; // ECMA-48 RIS (reset to initial state)
    (void)!xwrite_all(STDOUT_FILENO, reset, sizeof(reset) - 1);
    term_cooked();
    need_term_reset_on_fatal_error = 0;
}

void fatal_error_cleanup(void)
{
    cleanup();
}

#if ASAN_ENABLED == 1
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
    #if ASAN_ENABLED == 1
        (void)!fputs("\nStack trace:\n", stderr);
        __sanitizer_print_stack_trace();
    #endif
}

#if DEBUG_ASSERTIONS_ENABLED
noreturn void bug(const char *file, int line, const char *func, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    const char *str = likely(r >= 0) ? buf : "??";
    log_msg(LOG_LEVEL_CRITICAL, file, line, "BUG in %s(): '%s'", func, str);
    cleanup();

    (void)!fprintf(stderr, "\n%s:%d: **BUG** in %s(): '%s'\n", file, line, func, str);
    print_stack_trace();
    (void)!fflush(stderr);
    abort();
}
#endif

// Error handler for unrecoverable system errors during runtime
noreturn void fatal_error(const char *msg, SystemErrno err)
{
    LOG_CRITICAL("%s: %s", msg, strerror(err));
    cleanup();
    errno = err;
    perror(msg);
    print_stack_trace();
    abort();
}
