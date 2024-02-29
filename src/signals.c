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

// NOLINTNEXTLINE(*-avoid-non-const-global-variables)
volatile sig_atomic_t resized = 0;

typedef void (*SignalHandler)(int signum);

typedef enum {
    SD_IGNORED,
    SD_DEFAULT,
    SD_FATAL,
    SD_WINCH,
} SignalDispositionType;

#define sig(name, type) {#name, STRLEN(#name), SIG ## name, type}

static const struct {
    char name[7];
    uint8_t name_len;
    int signum;
    SignalDispositionType type;
} signals[] = {
    sig(INT,  SD_IGNORED), // Terminal interrupt (see: VINTR in termios(3))
    sig(QUIT, SD_IGNORED), // Terminal quit (see: VQUIT in termios(3))
    sig(TSTP, SD_IGNORED), // Terminal stop (see: VSUSP in termios(3))
    sig(XFSZ, SD_IGNORED), // File size limit exceeded (see: RLIMIT_FSIZE in getrlimit(3))
    sig(PIPE, SD_IGNORED), // Broken pipe (see: EPIPE error in write(3))
    sig(USR1, SD_IGNORED), // User signal 1 (terminates by default, for no good reason)
    sig(USR2, SD_IGNORED), // User signal 2 (as above)

    sig(ABRT, SD_DEFAULT), // Terminate (cleanup already done)
    sig(CHLD, SD_DEFAULT), // Ignore (see: wait(3))
    sig(URG,  SD_DEFAULT), // Ignore
    sig(TTIN, SD_DEFAULT), // Stop
    sig(TTOU, SD_DEFAULT), // Stop
    sig(CONT, SD_DEFAULT), // Continue

    sig(BUS,  SD_FATAL),
    sig(FPE,  SD_FATAL),
    sig(ILL,  SD_FATAL),
    sig(SEGV, SD_FATAL),
    sig(SYS,  SD_FATAL),
    sig(TRAP, SD_FATAL),
    sig(XCPU, SD_FATAL),
    sig(ALRM, SD_FATAL),
    sig(HUP,  SD_FATAL),
    sig(TERM, SD_FATAL),
    sig(VTALRM, SD_FATAL),

    #ifdef SIGPROF
        sig(PROF, SD_FATAL),
    #endif

    #ifdef SIGEMT
        sig(EMT, SD_FATAL),
    #endif

    #if defined(SIGWINCH)
        sig(WINCH, SD_WINCH),
    #endif
};

UNITTEST {
    CHECK_STRUCT_ARRAY(signals, name);
}

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

    for (size_t i = 0; i < ARRAYLEN(signals); i++) {
        if (signum == signals[i].signum) {
            memcpy(buf, signals[i].name, signals[i].name_len + 1);
            return 0;
        }
    }

    return -1;
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
