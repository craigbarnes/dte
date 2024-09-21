#include "feature.h"
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
#include "util/xstring.h"

volatile sig_atomic_t resized = 0; // NOLINT(*-avoid-non-const-global-variables)

static void handle_sigwinch(int UNUSED_ARG(signum))
{
    resized = 1;
}

static const struct {
    char name[10];
    int signum;
} signames[] = {
    {"SIGINT", SIGINT},
    {"SIGQUIT", SIGQUIT},
    {"SIGTSTP", SIGTSTP},
    {"SIGXFSZ", SIGXFSZ},
    {"SIGPIPE", SIGPIPE},
    {"SIGUSR1", SIGUSR1},
    {"SIGUSR2", SIGUSR2},
    {"SIGABRT", SIGABRT},
    {"SIGCHLD", SIGCHLD},
    {"SIGURG", SIGURG},
    {"SIGTTIN", SIGTTIN},
    {"SIGTTOU", SIGTTOU},
    {"SIGCONT", SIGCONT},
    {"SIGBUS", SIGBUS},
    {"SIGFPE", SIGFPE},
    {"SIGILL", SIGILL},
    {"SIGSEGV", SIGSEGV},
    {"SIGSYS", SIGSYS},
    {"SIGTRAP", SIGTRAP},
    {"SIGXCPU", SIGXCPU},
    {"SIGALRM", SIGALRM},
    {"SIGHUP", SIGHUP},
    {"SIGTERM", SIGTERM},
    {"SIGVTALRM", SIGVTALRM},
#ifdef SIGPROF
    {"SIGPROF", SIGPROF},
#endif
#ifdef SIGEMT
    {"SIGEMT", SIGEMT},
#endif
#ifdef SIGWINCH
    {"SIGWINCH", SIGWINCH},
#endif
};

UNITTEST {
    CHECK_STRUCT_ARRAY(signames, name);
}

static const char *signum_to_str(int signum)
{
#if defined(SIGRTMIN) && defined(SIGRTMAX)
    if (signum >= SIGRTMIN && signum <= SIGRTMAX) {
        return "realtime signal";
    }
#endif

    for (size_t i = 0; i < ARRAYLEN(signames); i++) {
        if (signum == signames[i].signum) {
            return signames[i].name;
        }
    }

    return "unknown signal";
}

static void xsigaction(int sig, const struct sigaction *action)
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

void set_sigwinch_handler(void)
{
#ifdef SIGWINCH
    struct sigaction action = {.sa_handler = handle_sigwinch};
    sigemptyset(&action.sa_mask);
    LOG_INFO("setting SIGWINCH handler");
    xsigaction(SIGWINCH, &action);
#endif
}

void set_basic_signal_dispositions(void)
{
    // Ignored signals
    struct sigaction action = {.sa_handler = SIG_IGN};
    sigemptyset(&action.sa_mask);
    xsigaction(SIGINT, &action);  // Terminal interrupt (see: VINTR in termios(3))
    xsigaction(SIGQUIT, &action); // Terminal quit (see: VQUIT in termios(3))
    xsigaction(SIGTSTP, &action); // Terminal stop (see: VSUSP in termios(3))
    xsigaction(SIGTTIN, &action); // Background process attempting read (see: tcsetpgrp(3))
    xsigaction(SIGTTOU, &action); // Background process attempting write
    xsigaction(SIGXFSZ, &action); // File size limit exceeded (see: RLIMIT_FSIZE in getrlimit(3))
    xsigaction(SIGPIPE, &action); // Broken pipe (see: EPIPE error in write(3))
    xsigaction(SIGUSR1, &action); // User signal 1 (terminates by default, for no good reason)
    xsigaction(SIGUSR2, &action); // User signal 2 (as above)

    // Default signals
    action.sa_handler = SIG_DFL;
    xsigaction(SIGABRT, &action); // Terminate (cleanup already done)
    xsigaction(SIGCHLD, &action); // Ignore (see: wait(3))
    xsigaction(SIGURG, &action);  // Ignore
    xsigaction(SIGCONT, &action); // Continue

    // Set signal mask explicitly, in case the parent process exec'd us
    // with blocked signals (bad practice, but not uncommon)
    sigset_t mask, prev_mask;
    if (sigemptyset(&mask) || sigprocmask(SIG_SETMASK, &mask, &prev_mask)) {
        LOG_ERRNO("setting signal mask failed");
        return;
    }

#if HAVE_SIGISEMPTYSET
    if (!sigisemptyset(&prev_mask)) {
        LOG_WARNING("non-empty signal mask was inherited (and cleared)");
    }
#endif
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
    _exit(EC_OS_ERROR);
}

/*
 * "A program that uses these functions should be written to catch all
 * signals and take other appropriate actions to ensure that when the
 * program terminates, whether planned or not, the terminal device's
 * state is restored to its original state."
 *
 * (https://pubs.opengroup.org/onlinepubs/9699919799/functions/tcgetattr.html)
 */
void set_fatal_signal_handlers(void)
{
    struct sigaction action = {.sa_handler = handle_fatal_signal};
    if (sigfillset(&action.sa_mask) != 0) {
        // This can never happen in glibc/musl/OpenBSD and there's no
        // sane reason why it ever should, but we return here instead
        // of registering signal handlers without a correct sa_mask.
        // Not having terminal cleanup for fatal signals is merely
        // inconvenient, whereas an incorrect mask could lead to UB.
        LOG_ERRNO("sigfillset");
        return;
    }

    xsigaction(SIGHUP, &action);
    xsigaction(SIGTERM, &action);
    xsigaction(SIGALRM, &action);
    xsigaction(SIGSEGV, &action);
    xsigaction(SIGBUS, &action);
    xsigaction(SIGTRAP, &action);
    xsigaction(SIGFPE, &action);
    xsigaction(SIGILL, &action);
    xsigaction(SIGSYS, &action);
    xsigaction(SIGXCPU, &action);

#ifdef SIGPROF
    xsigaction(SIGPROF, &action);
#endif

#ifdef SIGEMT
    xsigaction(SIGEMT, &action);
#endif

    // "The default actions for the realtime signals in the range SIGRTMIN
    // to SIGRTMAX shall be to terminate the process abnormally."
    // (POSIX.1-2017 §2.4.3)
#if defined(SIGRTMIN) && defined(SIGRTMAX)
    for (int s = SIGRTMIN, max = SIGRTMAX; s <= max; s++) {
        xsigaction(s, &action);
    }
#endif
}
