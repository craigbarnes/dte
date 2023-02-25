#include "compat.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "signals.h"
#include "util/debug.h"
#include "util/exitcode.h"
#include "util/log.h"
#include "util/macros.h"

volatile sig_atomic_t resized = 0;

static const int ignored_signals[] = {
    SIGINT,  // Terminal interrupt (see: VINTR in termios(3))
    SIGQUIT, // Terminal quit (see: VQUIT in termios(3))
    SIGTSTP, // Terminal stop (see: VSUSP in termios(3))
    SIGXFSZ, // File size limit exceeded (see: RLIMIT_FSIZE in getrlimit(3))
    SIGPIPE, // Broken pipe (see: EPIPE error in write(3))
    SIGUSR1, // User signal 1 (terminates by default, for no good reason)
    SIGUSR2, // User signal 2 (as above)
};

static const int default_signals[] = {
    SIGABRT, // Terminate (cleanup already done)
    SIGCHLD, // Ignore (see: wait(3))
    SIGURG,  // Ignore
    SIGTTIN, // Stop
    SIGTTOU, // Stop
    SIGCONT, // Continue
};

static const int fatal_signals[] = {
    SIGBUS,
    SIGFPE,
    SIGILL,
    SIGSEGV,
    SIGSYS,
    SIGTRAP,
    SIGXCPU,
    SIGALRM,
    SIGVTALRM,
    SIGHUP,
    SIGTERM,
#ifdef SIGPROF
    SIGPROF,
#endif
#ifdef SIGEMT
    SIGEMT,
#endif
};

void handle_sigwinch(int UNUSED_ARG(signum))
{
    resized = 1;
}

static noreturn COLD void handle_fatal_signal(int signum)
{
    LOG_CRITICAL("received signal %d (%s)", signum, strsignal(signum));

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

// strsignal(3) is fine in situations where a signal is being reported
// as terminating a process, but it tends to be confusing in most other
// circumstances, where the signal name (not description) is usually
// clearer
static const char *signum_to_str(int signum)
{
#if HAVE_SIG2STR
    static char buf[SIG2STR_MAX + 3];
    if (sig2str(signum, buf + 3) == 0) {
        return memcpy(buf, "SIG", 3);
    }
#elif HAVE_SIGABBREV_NP
    static char buf[16];
    const char *abbr = sigabbrev_np(signum);
    if (abbr && memccpy(buf + 3, abbr, '\0', sizeof(buf) - 3)) {
        return memcpy(buf, "SIG", 3);
    }
#endif

    const char *str = strsignal(signum);
    return likely(str) ? str : "??";
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
    for (size_t i = 0; i < ARRAYLEN(fatal_signals); i++) {
        do_sigaction(fatal_signals[i], &action);
    }

    // "The default actions for the realtime signals in the range SIGRTMIN
    // to SIGRTMAX shall be to terminate the process abnormally."
    // (POSIX.1-2017 §2.4.3)
#if defined(SIGRTMIN) && defined(SIGRTMAX)
    for (int s = SIGRTMIN, max = SIGRTMAX; s <= max; s++) {
        do_sigaction(s, &action);
    }
#endif

    action.sa_handler = SIG_IGN;
    for (size_t i = 0; i < ARRAYLEN(ignored_signals); i++) {
        do_sigaction(ignored_signals[i], &action);
    }

    action.sa_handler = SIG_DFL;
    for (size_t i = 0; i < ARRAYLEN(default_signals); i++) {
        do_sigaction(default_signals[i], &action);
    }

#if defined(SIGWINCH)
    LOG_INFO("setting SIGWINCH handler");
    action.sa_handler = handle_sigwinch;
    do_sigaction(SIGWINCH, &action);
#endif

    // Set signal mask explicitly, to avoid any possibility of
    // inheriting blocked signals
    sigset_t mask;
    sigemptyset(&mask);
    sigprocmask(SIG_SETMASK, &mask, NULL);
}
