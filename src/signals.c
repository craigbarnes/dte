#include "compat.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "signals.h"
#include "util/array.h"
#include "util/debug.h"
#include "util/exitcode.h"
#include "util/log.h"
#include "util/macros.h"
#include "util/str-util.h"

// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
volatile sig_atomic_t resized = 0;

typedef void (*SignalHandler)(int signum);

typedef enum {
    SD_IGNORED,
    SD_DEFAULT,
    SD_FATAL,
    SD_WINCH,
} SignalDispositionType;

#define entry(name, type) {"SIG" #name, type, SIG ## name}
#define ignore(name) entry(name, SD_IGNORED)
#define reset(name) entry(name, SD_DEFAULT)
#define fatal(name) entry(name, SD_FATAL)
#define winch(name) entry(name, SD_WINCH)

static const struct {
    char name[10];
    uint8_t type; // SignalDispositionType
    int signum;
} signals[] = {
    ignore(INT),  // Terminal interrupt (see: VINTR in termios(3))
    ignore(QUIT), // Terminal quit (see: VQUIT in termios(3))
    ignore(TSTP), // Terminal stop (see: VSUSP in termios(3))
    ignore(XFSZ), // File size limit exceeded (see: RLIMIT_FSIZE in getrlimit(3))
    ignore(PIPE), // Broken pipe (see: EPIPE error in write(3))
    ignore(USR1), // User signal 1 (terminates by default, for no good reason)
    ignore(USR2), // User signal 2 (as above)

    reset(ABRT), // Terminate (cleanup already done)
    reset(CHLD), // Ignore (see: wait(3))
    reset(URG),  // Ignore
    reset(TTIN), // Stop
    reset(TTOU), // Stop
    reset(CONT), // Continue

    fatal(BUS),
    fatal(FPE),
    fatal(ILL),
    fatal(SEGV),
    fatal(SYS),
    fatal(TRAP),
    fatal(XCPU),
    fatal(ALRM),
    fatal(HUP),
    fatal(TERM),
    fatal(VTALRM),

    #ifdef SIGPROF
        fatal(PROF),
    #endif
    #ifdef SIGEMT
        fatal(EMT),
    #endif
    #ifdef SIGWINCH
        winch(WINCH),
    #endif
};

UNITTEST {
    CHECK_STRUCT_ARRAY(signals, name);
    BUG_ON(!xstreq(signals[0].name, "SIGINT")); // NOLINT(*-assert-side-effect)
    BUG_ON(signals[0].signum != SIGINT);
    BUG_ON(signals[0].type != SD_IGNORED);
}

static const char *signum_to_str(int signum)
{
#if defined(SIGRTMIN) && defined(SIGRTMAX)
    if (signum >= SIGRTMIN && signum <= SIGRTMAX) {
        return "realtime signal";
    }
#endif

    for (size_t i = 0; i < ARRAYLEN(signals); i++) {
        if (signum == signals[i].signum) {
            return signals[i].name;
        }
    }

    return "unknown signal";
}

static noreturn COLD void handle_fatal_signal(int signum)
{
    const char *signame = signum_to_str(signum);
    log_write(LOG_LEVEL_CRITICAL, signame, strlen(signame));

    // If `signum` is SIGHUP, there's no point in trying to clean up the
    // state of the (disconnected) terminal
    if (signum != SIGHUP) {
        fatal_error_cleanup();
    }

    // Restore and unblock `signum` and then re-raise it, to ensure the
    // termination status (as seen by e.g. waitpid(3) in the parent) is
    // set appropriately
    struct sigaction sa = {.sa_handler = SIG_DFL};
    if (
        sigemptyset(&sa.sa_mask) == 0
        && sigaction(signum, &sa, NULL) == 0
        && sigaddset(&sa.sa_mask, signum) == 0
        && sigprocmask(SIG_UNBLOCK, &sa.sa_mask, NULL) == 0
    ) {
        raise(signum);
    }

    // This is here just to make extra certain the handler never returns.
    // If everything is working correctly, this code should be unreachable.
    raise(SIGKILL);
    _exit(EX_OSERR);
}

static void handle_sigwinch(int UNUSED_ARG(signum))
{
    resized = 1;
}

static void do_sigaction(int sig, const struct sigaction *action)
{
    struct sigaction old_action;
    if (unlikely(sigaction(sig, action, &old_action) != 0)) {
        const char *err = strerror(errno);
        LOG_ERROR("failed to set disposition for signal %d: %s", sig, err);
        return;
    }
    if (unlikely(old_action.sa_handler == SIG_IGN)) {
        const char *str = signum_to_str(sig);
        LOG_WARNING("ignored signal was inherited: %d (%s)", sig, str);
    }
}

static SignalHandler get_sig_handler(SignalDispositionType type)
{
    switch (type) {
    case SD_IGNORED:
        return SIG_IGN;
    case SD_DEFAULT:
        return SIG_DFL;
    case SD_WINCH:
        LOG_INFO("setting SIGWINCH handler");
        return handle_sigwinch;
    case SD_FATAL:
        return handle_fatal_signal;
    }

    BUG("invalid signal disposition type: %d", (int)type);
    return SIG_DFL;
}

/*
 * "A program that uses these functions should be written to catch all
 * signals and take other appropriate actions to ensure that when the
 * program terminates, whether planned or not, the terminal device's
 * state is restored to its original state."
 *
 * (https://pubs.opengroup.org/onlinepubs/9699919799/functions/tcgetattr.html)
 */
void set_signal_handlers(void)
{
    struct sigaction action = {.sa_handler = handle_fatal_signal};
    sigfillset(&action.sa_mask);

    // "The default actions for the realtime signals in the range SIGRTMIN
    // to SIGRTMAX shall be to terminate the process abnormally."
    // (POSIX.1-2017 §2.4.3)
#if defined(SIGRTMIN) && defined(SIGRTMAX)
    for (int s = SIGRTMIN, max = SIGRTMAX; s <= max; s++) {
        do_sigaction(s, &action);
    }
#endif

    for (size_t i = 0; i < ARRAYLEN(signals); i++) {
        action.sa_handler = get_sig_handler(signals[i].type);
        do_sigaction(signals[i].signum, &action);
    }

    // Set signal mask explicitly, to avoid any possibility of
    // inheriting blocked signals
    sigset_t mask;
    sigemptyset(&mask);
    sigprocmask(SIG_SETMASK, &mask, NULL);
}
