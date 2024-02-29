#include "compat.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "signals.h"
#include "util/debug.h"
#include "util/exitcode.h"
#include "util/log.h"
#include "util/macros.h"

// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
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

enum {
    KNOWN_SIG_MAX = STRLEN("VTALRM") + 2,
#if HAVE_SIG2STR
    XSIG2STR_MAX = MAX(KNOWN_SIG_MAX, SIG2STR_MAX + 1),
#else
    XSIG2STR_MAX = KNOWN_SIG_MAX,
#endif
    SIGNAME_MAX = MAX(16, STRLEN("SIG") + XSIG2STR_MAX),
};

static int xsig2str(int signum, char buf[XSIG2STR_MAX])
{
#if HAVE_SIG2STR
    if (sig2str(signum, buf) == 0) {
        return 0;
    }
#elif HAVE_SIGABBREV_NP
    const char *abbr = sigabbrev_np(signum);
    if (abbr) {
        return memccpy(buf, abbr, '\0', XSIG2STR_MAX - 1) ? 0 : -1;
    }
#endif

    const char *name;
    switch (signum) {
    case SIGINT: name = "INT"; break;
    case SIGQUIT: name = "QUIT"; break;
    case SIGTSTP: name = "TSTP"; break;
    case SIGXFSZ: name = "XFSZ"; break;
    case SIGPIPE: name = "PIPE"; break;
    case SIGUSR1: name = "USR1"; break;
    case SIGUSR2: name = "USR2"; break;
    case SIGABRT: name = "ABRT"; break;
    case SIGCHLD: name = "CHLD"; break;
    case SIGURG: name = "URG"; break;
    case SIGTTIN: name = "TTIN"; break;
    case SIGTTOU: name = "TTOU"; break;
    case SIGCONT: name = "CONT"; break;
    case SIGBUS: name = "BUS"; break;
    case SIGFPE: name = "FPE"; break;
    case SIGILL: name = "ILL"; break;
    case SIGSEGV: name = "SEGV"; break;
    case SIGSYS: name = "SYS"; break;
    case SIGTRAP: name = "TRAP"; break;
    case SIGXCPU: name = "XCPU"; break;
    case SIGALRM: name = "ALRM"; break;
    case SIGVTALRM: name = "VTALRM"; break;
    case SIGHUP: name = "HUP"; break;
    case SIGTERM: name = "TERM"; break;
    #ifdef SIGPROF
        case SIGPROF: name = "PROF"; break;
    #endif
    #ifdef SIGEMT
        case SIGEMT: name = "EMT"; break;
    #endif
    default: return -1;
    }

    memcpy(buf, name, strlen(name) + 1);
    return 0;
}

// strsignal(3) is fine in situations where a signal is being reported
// as terminating a process, but it tends to be confusing in most other
// circumstances, where the signal name (not description) is usually
// clearer
static const char *signum_to_str(int signum, char buf[SIGNAME_MAX])
{
    if (xsig2str(signum, buf + 3) == 0) {
        return memcpy(buf, "SIG", 3);
    }

#if defined(SIGRTMIN) && defined(SIGRTMAX)
    if (signum >= SIGRTMIN && signum <= SIGRTMAX) {
        static const char rt[] = "realtime signal";
        static_assert(SIGNAME_MAX >= sizeof(rt));
        return memcpy(buf, rt, sizeof(rt));
    }
#endif

    static const char fallback[] = "unknown signal";
    static_assert(SIGNAME_MAX >= sizeof(fallback));
    return memcpy(buf, fallback, sizeof(fallback));
}

static noreturn COLD void handle_fatal_signal(int signum)
{
    char buf[SIGNAME_MAX];
    const char *signame = signum_to_str(signum, buf);
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
        char buf[SIGNAME_MAX];
        const char *str = signum_to_str(sig, buf);
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
