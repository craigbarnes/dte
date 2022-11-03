#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "block.h"
#include "command/serialize.h"
#include "commands.h"
#include "config.h"
#include "editor.h"
#include "error.h"
#include "file-history.h"
#include "frame.h"
#include "history.h"
#include "load-save.h"
#include "move.h"
#include "search.h"
#include "syntax/state.h"
#include "syntax/syntax.h"
#include "tag.h"
#include "terminal/input.h"
#include "terminal/mode.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "util/debug.h"
#include "util/exitcode.h"
#include "util/log.h"
#include "util/macros.h"
#include "util/path.h"
#include "util/ptr-array.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"
#include "util/xsnprintf.h"
#include "view.h"
#include "window.h"
#include "../build/version.h"

static void handle_sigcont(int UNUSED_ARG(signum))
{
    if (
        !editor.child_controls_terminal
        && editor.status != EDITOR_INITIALIZING
    ) {
        int saved_errno = errno;
        term_raw();
        ui_start(&editor);
        errno = saved_errno;
    }
}

static void term_cleanup(EditorState *e)
{
    set_fatal_error_cleanup_handler(NULL, NULL);
    if (!e->child_controls_terminal) {
        ui_end(e);
    }
}

static void cleanup_handler(void *userdata)
{
    term_cleanup(userdata);
}

static noreturn COLD void handle_fatal_signal(int signum)
{
    LOG_ERROR("received signal %d (%s)", signum, strsignal(signum));

    if (signum != SIGHUP) {
        term_cleanup(&editor);
    }

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

static void do_sigaction(int sig, const struct sigaction *action)
{
    int r = sigaction(sig, action, NULL);
    if (unlikely(r != 0)) {
        const char *err = strerror(errno);
        LOG_ERROR("failed to set disposition for signal %d: %s", sig, err);
    }
}

/*
 * "A program that uses these functions should be written to catch all
 * signals and take other appropriate actions to ensure that when the
 * program terminates, whether planned or not, the terminal device's
 * state is restored to its original state."
 *
 * (https://pubs.opengroup.org/onlinepubs/9699919799/functions/tcgetattr.html)
 *
 * Signals not handled by this function:
 * - SIGKILL, SIGSTOP (can't be caught or ignored)
 * - SIGPOLL, SIGPROF (marked "obsolete" in POSIX 2008)
 */
static void set_signal_handlers(void)
{
    static const int fatal_signals[] = {
        SIGBUS, SIGFPE, SIGILL, SIGSEGV,
        SIGSYS, SIGTRAP, SIGXCPU, SIGXFSZ,
        SIGALRM, SIGVTALRM,
        SIGHUP, SIGTERM,
#ifdef SIGEMT
        SIGEMT,
#endif
    };

    static const int ignored_signals[] = {
        SIGINT, SIGQUIT, SIGTSTP,
        SIGPIPE, SIGUSR1, SIGUSR2,
    };

    static const int default_signals[] = {
        SIGABRT, // Terminate (cleanup already done)
        SIGCHLD, // Ignore (see also: wait(3))
        SIGURG,  // Ignore
        SIGTTIN, // Stop
        SIGTTOU, // Stop
    };

    static const struct {
        int signum;
        void (*handler)(int);
    } handled_signals[] = {
        {SIGCONT, handle_sigcont},
#ifdef SIGWINCH
        {SIGWINCH, handle_sigwinch},
#endif
    };

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

    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < ARRAYLEN(handled_signals); i++) {
        action.sa_handler = handled_signals[i].handler;
        do_sigaction(handled_signals[i].signum, &action);
    }

    // Set signal mask explicitly, to avoid any possibility of
    // inheriting blocked signals
    sigset_t mask;
    sigemptyset(&mask);
    sigprocmask(SIG_SETMASK, &mask, NULL);
}

static ExitCode write_str(int fd, const char *str, size_t len)
{
    if (xwrite_all(fd, str, len) < 0) {
        perror("write");
        return EX_IOERR;
    }
    return EX_OK;
}

static ExitCode list_builtin_configs(void)
{
    String str = dump_builtin_configs();
    BUG_ON(!str.buffer);
    ExitCode e = write_str(STDOUT_FILENO, str.buffer, str.len);
    string_free(&str);
    return e;
}

static ExitCode dump_builtin_config(const char *name)
{
    const BuiltinConfig *cfg = get_builtin_config(name);
    if (!cfg) {
        fprintf(stderr, "Error: no built-in config with name '%s'\n", name);
        return EX_USAGE;
    }
    return write_str(STDOUT_FILENO, cfg->text.data, cfg->text.length);
}

static ExitCode lint_syntax(const char *filename)
{
    int err;
    const Syntax *s = load_syntax_file(&editor, filename, CFG_MUST_EXIST, &err);
    if (s) {
        const size_t n = s->states.count;
        const char *p = (n > 1) ? "s" : "";
        printf("OK: loaded syntax '%s' with %zu state%s\n", s->name, n, p);
    } else if (err == EINVAL) {
        error_msg("%s: no default syntax found", filename);
    }
    return get_nr_errors() ? EX_DATAERR : EX_OK;
}

static ExitCode showkey_loop(Terminal *term)
{
    if (!term_raw()) {
        perror("tcsetattr");
        return EX_IOERR;
    }

    TermOutputBuffer *obuf = &term->obuf;
    term_enable_private_modes(term);
    term_add_literal(obuf, "Press any key combination, or use Ctrl+D to exit\r\n");
    term_output_flush(obuf);

    bool loop = true;
    while (loop) {
        KeyCode key = term_read_key(term, 100);
        switch (key) {
        case KEY_NONE:
        case KEY_IGNORE:
            continue;
        case KEY_BRACKETED_PASTE:
        case KEY_DETECTED_PASTE:
            term_discard_paste(&term->ibuf, key == KEY_BRACKETED_PASTE);
            continue;
        case MOD_CTRL | 'd':
            loop = false;
        }
        const char *str = keycode_to_string(key);
        term_add_literal(obuf, "  ");
        term_add_bytes(obuf, str, strlen(str));
        term_add_literal(obuf, "\r\n");
        term_output_flush(obuf);
    }

    term_restore_private_modes(term);
    term_output_flush(obuf);
    term_cooked();
    return EX_OK;
}

static ExitCode init_std_fds(int std_fds[2])
{
    FILE *streams[3] = {stdin, stdout, stderr};
    for (int i = 0; i < ARRAYLEN(streams); i++) {
        if (isatty(i)) {
            continue;
        }

        if (i < STDERR_FILENO) {
            // Try to create a duplicate fd for redirected stdin/stdout; to
            // allow reading/writing after freopen(3) closes the original
            int fd = fcntl(i, F_DUPFD_CLOEXEC, 3);
            if (fd == -1 && errno != EBADF) {
                perror("fcntl");
                return EX_OSERR;
            }
            std_fds[i] = fd;
        }

        // Ensure standard streams are connected to the terminal during
        // editor operation, regardless of how they were redirected
        if (unlikely(!freopen("/dev/tty", i ? "w" : "r", streams[i]))) {
            const char *err = strerror(errno);
            fprintf(stderr, "Failed to open tty for fd %d: %s\n", i, err);
            return EX_IOERR;
        }

        int new_fd = fileno(streams[i]);
        if (unlikely(new_fd != i)) {
            // This should never happen in a single-threaded program.
            // freopen() should call fclose() followed by open() and
            // POSIX requires a successful call to open() to return the
            // lowest available file descriptor.
            fprintf(stderr, "freopen() changed fd from %d to %d\n", i, new_fd);
            return EX_OSERR;
        }
    }

    return EX_OK;
}

static Buffer *init_std_buffer(EditorState *e, int fds[2])
{
    const char *name = NULL;
    Buffer *b = NULL;

    if (fds[STDIN_FILENO] >= 3) {
        Encoding enc = encoding_from_type(UTF8);
        b = buffer_new(&e->buffers, &e->options, &enc);
        if (read_blocks(b, fds[STDIN_FILENO])) {
            name = "(stdin)";
            b->temporary = true;
        } else {
            error_msg("Unable to read redirected stdin");
            remove_and_free_buffer(&e->buffers, b);
            b = NULL;
        }
    }

    if (fds[STDOUT_FILENO] >= 3) {
        if (!b) {
            b = open_empty_buffer(&e->buffers, &e->options);
            name = "(stdout)";
        } else {
            name = "(stdin|stdout)";
        }
        b->stdout_buffer = true;
        b->temporary = true;
    }

    BUG_ON(!b != !name);
    if (name) {
        set_display_filename(b, xstrdup(name));
    }

    return b;
}

static ExitCode init_logging(const char *filename, const char *log_level_str)
{
    if (!filename || filename[0] == '\0') {
        return EX_OK;
    }

    LogLevel req_level = log_level_from_str(log_level_str);
    if (req_level == LOG_LEVEL_NONE) {
        return EX_OK;
    }

    LogLevel got_level = log_open(filename, req_level);
    if (got_level == LOG_LEVEL_NONE) {
        const char *err = strerror(errno);
        fprintf(stderr, "Failed to open $DTE_LOG (%s): %s\n", filename, err);
        return EX_IOERR;
    }

    const char *got_level_str = log_level_to_str(got_level);
    if (got_level != req_level) {
        const char *r = log_level_to_str(req_level);
        const char *g = got_level_str;
        LOG_WARNING("log level '%s' unavailable; falling back to '%s'", r, g);
    }

    LOG_INFO("logging to '%s' (level: %s)", filename, got_level_str);

    struct utsname u;
    if (likely(uname(&u) >= 0)) {
        LOG_INFO("system: %s/%s %s", u.sysname, u.machine, u.release);
    } else {
        LOG_ERROR("uname() failed: %s", strerror(errno));
    }
    return EX_OK;
}

static const char copyright[] =
    "dte " VERSION "\n"
    "(C) 2013-2022 Craig Barnes\n"
    "(C) 2010-2015 Timo Hirvonen\n"
    "This program is free software; you can redistribute and/or modify\n"
    "it under the terms of the GNU General Public License version 2\n"
    "<https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n";

static const char usage[] =
    "Usage: %s [OPTIONS] [[+LINE] FILE]...\n\n"
    "Options:\n"
    "   -c COMMAND  Run COMMAND after editor starts\n"
    "   -t CTAG     Jump to source location of CTAG\n"
    "   -r RCFILE   Read user config from RCFILE instead of ~/.dte/rc\n"
    "   -s FILE     Validate dte-syntax commands in FILE and exit\n"
    "   -b NAME     Print built-in config matching NAME and exit\n"
    "   -B          Print list of built-in config names and exit\n"
    "   -H          Don't load or save history files\n"
    "   -R          Don't read user config file\n"
    "   -K          Start editor in \"showkey\" mode\n"
    "   -h          Display help summary and exit\n"
    "   -V          Display version number and exit\n"
    "\n";

int main(int argc, char *argv[])
{
    static const char optstring[] = "hBHKRVb:c:t:r:s:";
    const char *tag = NULL;
    const char *rc = NULL;
    PointerArray commands = PTR_ARRAY_INIT;
    bool read_rc = true;
    bool use_showkey = false;
    bool load_and_save_history = true;
    set_print_errors_to_stderr(true);

    for (int ch; (ch = getopt(argc, argv, optstring)) != -1; ) {
        switch (ch) {
        case 'c':
            ptr_array_append(&commands, optarg);
            break;
        case 't':
            tag = optarg;
            break;
        case 'r':
            rc = optarg;
            break;
        case 's':
            return lint_syntax(optarg);
        case 'R':
            read_rc = false;
            break;
        case 'b':
            return dump_builtin_config(optarg);
        case 'B':
            return list_builtin_configs();
        case 'H':
            load_and_save_history = false;
            break;
        case 'K':
            use_showkey = true;
            goto loop_break;
        case 'V':
            return write_str(STDOUT_FILENO, copyright, sizeof(copyright));
        case 'h':
            printf(usage, (argv[0] && argv[0][0]) ? argv[0] : "dte");
            return EX_OK;
        case '?':
        default:
            return EX_USAGE;
        }
    }

loop_break:;

    // This must be done before calling init_logging(), otherwise an
    // invocation like e.g. `DTE_LOG=/dev/pts/2 dte 0<&-` could
    // cause the logging fd to be opened as STDIN_FILENO.
    int std_fds[2] = {-1, -1};
    ExitCode r = init_std_fds(std_fds);
    if (unlikely(r != EX_OK)) {
        return r;
    }

    r = init_logging(getenv("DTE_LOG"), getenv("DTE_LOG_LEVEL"));
    if (unlikely(r != EX_OK)) {
        return r;
    }

    EditorState *e = init_editor_state();

    const char *term_name = getenv("TERM");
    if (!term_name || term_name[0] == '\0') {
        fputs("Error: $TERM not set\n", stderr);
        // This is considered a "usage" error, because the program
        // must be started from a properly configured terminal
        return EX_USAGE;
    }

    if (!term_mode_init()) {
        perror("tcgetattr");
        return EX_IOERR;
    }

    Terminal *term = &e->terminal;
    term_init(term, term_name);

    if (use_showkey) {
        return showkey_loop(term);
    }

    Buffer *std_buffer = init_std_buffer(e, std_fds);
    bool have_stdout_buffer = std_buffer && std_buffer->stdout_buffer;

    // Create this early (needed if "lock-files" is true)
    const char *cfgdir = e->user_config_dir;
    BUG_ON(!cfgdir);
    if (mkdir(cfgdir, 0755) != 0 && errno != EEXIST) {
        error_msg("Error creating %s: %s", cfgdir, strerror(errno));
        load_and_save_history = false;
        e->options.lock_files = false;
    }

    term_save_title(term);
    exec_builtin_rc(&e->colors, e->terminal.color_type);

    if (read_rc) {
        ConfigFlags flags = CFG_NOFLAGS;
        char buf[4096];
        if (rc) {
            flags |= CFG_MUST_EXIST;
        } else {
            xsnprintf(buf, sizeof buf, "%s/%s", cfgdir, "rc");
            rc = buf;
        }
        LOG_INFO("loading configuration from %s", rc);
        read_config(&normal_commands, rc, flags);
    }

    update_all_syntax_colors(&e->syntaxes, &e->colors);

    Window *window = new_window(e);
    e->window = window;
    e->root_frame = new_root_frame(window);

    set_signal_handlers();
    set_fatal_error_cleanup_handler(cleanup_handler, e);

    if (load_and_save_history) {
        file_history_load(&e->file_history, path_join(cfgdir, "file-history"));
        history_load(&e->command_history, path_join(cfgdir, "command-history"));
        history_load(&e->search_history, path_join(cfgdir, "search-history"));
        if (e->search_history.last) {
            search_set_regexp(&e->search, e->search_history.last->text);
        }
    }

    set_print_errors_to_stderr(false);

    // Initialize terminal but don't update screen yet. Also display
    // "Press any key to continue" prompt if there were any errors
    // during reading configuration files.
    if (!term_raw()) {
        perror("tcsetattr");
        return EX_IOERR;
    }
    if (get_nr_errors()) {
        any_key(term, e->options.esc_timeout);
        clear_error();
    }

    e->status = EDITOR_RUNNING;

    for (size_t i = optind, line = 0, col = 0; i < argc; i++) {
        const char *str = argv[i];
        if (str[0] == '+' && ascii_isdigit(str[1]) && line == 0) {
            if (!str_to_filepos(str + 1, &line, &col)) {
                error_msg("Invalid file position: '%s'", str);
            }
        } else {
            View *v = window_open_buffer(window, str, false, NULL);
            if (line > 0) {
                set_view(e, v);
                move_to_line(v, line);
                line = 0;
                if (col > 0) {
                    move_to_column(v, col);
                    col = 0;
                }
            }
        }
    }

    if (std_buffer) {
        window_add_buffer(window, std_buffer);
    }

    View *empty_buffer = NULL;
    if (window->views.count == 0) {
        empty_buffer = window_open_empty_buffer(window);
        BUG_ON(!empty_buffer);
        BUG_ON(window->views.count != 1);
        BUG_ON(empty_buffer != window->views.ptrs[0]);
    }

    set_view(e, window->views.ptrs[0]);
    ui_start(e);

    bool cflag = false;
    if (commands.count > 0) {
        for (size_t i = 0, n = commands.count; i < n; i++) {
            const char *command = commands.ptrs[i];
            handle_command(&normal_commands, command, false);
        }
        ptr_array_free_array(&commands);
        cflag = true;
    }

    if (tag) {
        tag_lookup(&e->tagfile, tag, NULL, &e->messages);
        activate_current_message(e);
    }

    if (
        // If window_open_empty_buffer() was called above
        empty_buffer
        // ...and no commands were executed via the "-c" flag
        && !cflag
        // ...and a file was opened via the "-t" flag
        && tag && window->views.count > 1
    ) {
        // Close the empty buffer, leaving just the buffer opened via "-t"
        remove_view(empty_buffer);
        empty_buffer = NULL;
    }

    if (cflag || tag) {
        normal_update(e);
    }

    main_loop(e);

    BUG_ON(e->status != EDITOR_EXITING);
    term_restore_title(term);
    ui_end(e);
    term_output_flush(&term->obuf);
    set_print_errors_to_stderr(true);

    // Unlock files and add files to file history
    remove_frame(e, e->root_frame);

    if (load_and_save_history) {
        history_save(&e->command_history);
        history_save(&e->search_history);
        file_history_save(&e->file_history);
    }

    if (have_stdout_buffer) {
        int fd = std_fds[STDOUT_FILENO];
        Block *blk;
        block_for_each(blk, &std_buffer->blocks) {
            if (xwrite_all(fd, blk->data, blk->size) < 0) {
                const char *err = strerror(errno);
                error_msg("failed to write (stdout) buffer: %s", err);
                e->exit_code = EX_IOERR;
                break;
            }
        }
        free_blocks(std_buffer);
        free(std_buffer);
    }

    LOG_INFO("exiting with status %d", e->exit_code);
    log_close();
    return e->exit_code;
}
