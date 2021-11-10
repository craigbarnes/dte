#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
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
#include "terminal/input.h"
#include "terminal/mode.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "util/debug.h"
#include "util/exitcode.h"
#include "util/macros.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"
#include "view.h"
#include "window.h"

static void handle_sigcont(int UNUSED_ARG(signum))
{
    if (
        !editor.child_controls_terminal
        && editor.status != EDITOR_INITIALIZING
    ) {
        term_raw();
        ui_start();
    }
}

#ifdef SIGWINCH
static void handle_sigwinch(int UNUSED_ARG(signum))
{
    editor.resized = true;
}
#endif

static void term_cleanup(void)
{
    set_fatal_error_cleanup_handler(NULL);
    if (!editor.child_controls_terminal) {
        ui_end();
    }
}

static noreturn COLD void handle_fatal_signal(int signum)
{
    DEBUG_LOG("Received signal %d (%s)", signum, strsignal(signum));

    if (signum != SIGHUP) {
        term_cleanup();
    }

    struct sigaction sa = {.sa_handler = SIG_DFL};
    sigemptyset(&sa.sa_mask);
    sigaction(signum, &sa, NULL);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signum);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    raise(signum);

    // This is here just to make extra certain the handler never returns.
    // If everything is working correctly, this code should be unreachable.
    raise(SIGKILL);
    _exit(EX_OSERR);
}

static void do_sigaction(int sig, const struct sigaction *action)
{
    if (sigaction(sig, action, NULL) != 0) {
        BUG("Failed to add handler for signal %d: %s", sig, strerror(errno));
    }
}

// Signals not handled by this function:
// * SIGKILL, SIGSTOP (can't be caught or ignored)
// * SIGPOLL, SIGPROF (marked "obsolete" in POSIX 2008)
static void set_signal_handlers(void)
{
    static const int fatal_signals[] = {
        SIGBUS, SIGFPE, SIGILL, SIGSEGV,
        SIGSYS, SIGTRAP, SIGXCPU, SIGXFSZ,
        SIGALRM, SIGVTALRM,
        SIGHUP, SIGTERM,
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
    for (size_t i = 0; i < ARRAY_COUNT(fatal_signals); i++) {
        do_sigaction(fatal_signals[i], &action);
    }

    action.sa_handler = SIG_IGN;
    for (size_t i = 0; i < ARRAY_COUNT(ignored_signals); i++) {
        do_sigaction(ignored_signals[i], &action);
    }

    action.sa_handler = SIG_DFL;
    for (size_t i = 0; i < ARRAY_COUNT(default_signals); i++) {
        do_sigaction(default_signals[i], &action);
    }

    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < ARRAY_COUNT(handled_signals); i++) {
        action.sa_handler = handled_signals[i].handler;
        do_sigaction(handled_signals[i].signum, &action);
    }
}

static ExitCode list_builtin_configs(void)
{
    String str = dump_builtin_configs();
    ssize_t n = xwrite(STDOUT_FILENO, str.buffer, str.len);
    string_free(&str);
    if (n < 0) {
        perror("write");
        return EX_IOERR;
    }
    return EX_OK;
}

static ExitCode dump_builtin_config(const char *name)
{
    const BuiltinConfig *cfg = get_builtin_config(name);
    if (!cfg) {
        fprintf(stderr, "Error: no built-in config with name '%s'\n", name);
        return EX_USAGE;
    }
    if (xwrite(STDOUT_FILENO, cfg->text.data, cfg->text.length) < 0) {
        perror("write");
        return EX_IOERR;
    }
    return EX_OK;
}

static ExitCode lint_syntax(const char *filename)
{
    int err;
    const Syntax *s = load_syntax_file(filename, CFG_MUST_EXIST, &err);
    if (s) {
        const size_t n = s->states.count;
        const char *p = (n > 1) ? "s" : "";
        printf("OK: loaded syntax '%s' with %zu state%s\n", s->name, n, p);
    } else if (err == EINVAL) {
        error_msg("%s: no default syntax found", filename);
    }
    return get_nr_errors() ? EX_DATAERR : EX_OK;
}

static ExitCode showkey_loop(void)
{
    if (!term_raw()) {
        perror("tcsetattr");
        return EX_IOERR;
    }
    TermOutputBuffer *obuf = &editor.obuf;
    term_add_string_view(obuf, terminal.control_codes.init);
    term_add_string_view(obuf, terminal.control_codes.keypad_on);
    term_add_literal(obuf, "Press any key combination, or use Ctrl+D to exit\r\n");
    term_output_flush(obuf);

    bool loop = true;
    while (loop) {
        KeyCode key = term_read_key();
        switch (key) {
        case KEY_NONE:
        case KEY_IGNORE:
            continue;
        case KEY_PASTE:
            term_discard_paste();
            continue;
        case MOD_CTRL | 'D':
            loop = false;
        }
        const char *str = keycode_to_string(key);
        term_add_literal(obuf, "  ");
        term_add_bytes(obuf, str, strlen(str));
        term_add_literal(obuf, "\r\n");
        term_output_flush(obuf);
    }

    term_add_string_view(obuf, terminal.control_codes.keypad_off);
    term_add_string_view(obuf, terminal.control_codes.deinit);
    term_output_flush(obuf);
    term_cooked();
    return EX_OK;
}

static void freopen_tty(FILE *stream, const char *mode, int fd)
{
    if (unlikely(!freopen("/dev/tty", mode, stream))) {
        const char *err = strerror(errno);
        fprintf(stderr, "Failed to open tty in mode '%s': %s\n", mode, err);
        exit(EX_IOERR);
    }

    int new_fd = fileno(stream);
    if (unlikely(new_fd != fd)) {
        // This should never happen in a single-threaded program.
        // freopen() should call fclose() followed by open() and
        // POSIX requires a successful call to open() to return the
        // lowest available file descriptor.
        fprintf(stderr, "freopen() changed fd from %d to %d\n", fd, new_fd);
        exit(EX_OSERR);
    }
}

static const char copyright[] =
    "(C) 2017-2021 Craig Barnes\n"
    "(C) 2010-2015 Timo Hirvonen\n"
    "This program is free software; you can redistribute and/or modify\n"
    "it under the terms of the GNU General Public License version 2\n"
    "<https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>.\n"
    "There is NO WARRANTY, to the extent permitted by law.";

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
    const char *command = NULL;
    bool read_rc = true;
    bool use_showkey = false;
    bool load_and_save_history = true;
    int ch;

    while ((ch = getopt(argc, argv, optstring)) != -1) {
        switch (ch) {
        case 'c':
            command = optarg;
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
            printf("dte %s\n", editor.version);
            puts(copyright);
            return EX_OK;
        case 'h':
            printf(usage, argv[0]);
            return EX_OK;
        case '?':
        default:
            return EX_USAGE;
        }
    }

loop_break:

    init_editor_state();

    Buffer *stdin_buffer = NULL;
    if (!isatty(STDIN_FILENO)) {
        Buffer *b = buffer_new(&editor.charset);
        if (read_blocks(b, STDIN_FILENO)) {
            set_display_filename(b, xmemdup_literal("(stdin)"));
            stdin_buffer = b;
            stdin_buffer->temporary = true;
        } else {
            if (errno != EBADF) {
                error_msg("Unable to read redirected stdin");
            }
            free_buffer(b);
        }
        freopen_tty(stdin, "r", STDIN_FILENO);
    }

    Buffer *stdout_buffer = NULL;
    int old_stdout_fd = -1;
    if (!isatty(STDOUT_FILENO)) {
        old_stdout_fd = fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3);
        if (old_stdout_fd == -1 && errno != EBADF) {
            perror("fcntl");
            return EX_OSERR;
        }
        freopen_tty(stdout, "w", STDOUT_FILENO);
        if (old_stdout_fd == -1) {
            // The call to fcntl(3) above failed with EBADF, meaning stdout was
            // most likely closed and there's no point opening a buffer for it
        } else if (stdin_buffer) {
            stdout_buffer = stdin_buffer;
            stdout_buffer->stdout_buffer = true;
            set_display_filename(stdout_buffer, xmemdup_literal("(stdin|stdout)"));
        } else {
            stdout_buffer = open_empty_buffer();
            set_display_filename(stdout_buffer, xmemdup_literal("(stdout)"));
            stdout_buffer->stdout_buffer = true;
            stdout_buffer->temporary = true;
        }
    }

    const char *term = getenv("TERM");
    if (!term || term[0] == '\0') {
        fputs("Error: $TERM not set\n", stderr);
        // This is considered a "usage" error, because the program
        // must be started from a properly configured terminal
        return EX_USAGE;
    }

    if (!term_mode_init()) {
        perror("tcgetattr");
        return EX_IOERR;
    }

    term_init(term);

    if (use_showkey) {
        return showkey_loop();
    }

    // Create this early. Needed if lock-files is true.
    const char *editor_dir = editor.user_config_dir;
    if (mkdir(editor_dir, 0755) != 0 && errno != EEXIST) {
        error_msg("Error creating %s: %s", editor_dir, strerror(errno));
        load_and_save_history = false;
        editor.options.lock_files = false;
    }

    term_save_title(&editor.obuf);
    exec_builtin_rc();

    if (read_rc) {
        ConfigFlags flags = CFG_NOFLAGS;
        if (rc) {
            flags |= CFG_MUST_EXIST;
        } else {
            rc = editor_file("rc");
        }
        DEBUG_LOG("loading configuration from %s", rc);
        read_config(&normal_commands, rc, flags);
    }

    update_all_syntax_colors(&editor.syntaxes);
    window = new_window();
    root_frame = new_root_frame(window);

    set_signal_handlers();
    set_fatal_error_cleanup_handler(term_cleanup);

    if (load_and_save_history) {
        file_history_load(&editor.file_history, editor_file("file-history"));
        history_load(&editor.command_history, editor_file("command-history"));
        history_load(&editor.search_history, editor_file("search-history"));
        if (editor.search_history.last) {
            search_set_regexp(&editor.search, editor.search_history.last->text);
        }
    }

    // Initialize terminal but don't update screen yet. Also display
    // "Press any key to continue" prompt if there were any errors
    // during reading configuration files.
    if (!term_raw()) {
        perror("tcsetattr");
        return EX_IOERR;
    }
    if (get_nr_errors()) {
        any_key();
        clear_error();
    }

    editor.status = EDITOR_RUNNING;

    for (int i = optind, lineno = 0; i < argc; i++) {
        if (argv[i][0] == '+' && lineno <= 0) {
            const char *const lineno_string = &argv[i][1];
            if (!str_to_int(lineno_string, &lineno) || lineno <= 0) {
                error_msg("Invalid line number: '%s'", lineno_string);
            }
        } else {
            View *v = window_open_buffer(window, argv[i], false, NULL);
            if (lineno > 0) {
                set_view(v);
                move_to_line(v, lineno);
                lineno = 0;
            }
        }
    }

    if (stdin_buffer) {
        window_add_buffer(window, stdin_buffer);
    }
    if (stdout_buffer && stdout_buffer != stdin_buffer) {
        window_add_buffer(window, stdout_buffer);
    }

    const View *empty_buffer = NULL;
    if (window->views.count == 0) {
        empty_buffer = window_open_empty_buffer(window);
    }

    set_view(window->views.ptrs[0]);
    ui_start();

    if (command) {
        handle_command(&normal_commands, command, false);
    }

    if (tag) {
        String s = string_new(8 + strlen(tag));
        string_append_literal(&s, "tag ");
        if (unlikely(tag[0] == '-')) {
            string_append_literal(&s, "-- ");
        }
        string_append_escaped_arg(&s, tag, true);
        handle_command(&normal_commands, string_borrow_cstring(&s), false);
        string_free(&s);
    }

    if (
        // If window_open_empty_buffer() was called above
        empty_buffer
        // ...and no commands were executed via the "-c" flag
        && !command
        // ...and a file was opened via the "-t" flag
        && tag && window->views.count > 1
    ) {
        // Close the empty buffer, leaving just the buffer opened via "-t"
        remove_view(window->views.ptrs[0]);
    }

    if (command || tag) {
        normal_update();
    }

    main_loop();

    term_restore_title(&editor.obuf);
    ui_end();
    term_output_flush(&editor.obuf);

    // Unlock files and add files to file history
    remove_frame(root_frame);

    if (load_and_save_history) {
        history_save(&editor.command_history);
        history_save(&editor.search_history);
        file_history_save(&editor.file_history);
    }

    if (stdout_buffer) {
        Block *blk;
        block_for_each(blk, &stdout_buffer->blocks) {
            if (xwrite(old_stdout_fd, blk->data, blk->size) < 0) {
                perror_msg("write");
                return EX_IOERR;
            }
        }
        free_blocks(stdout_buffer);
        free(stdout_buffer);
    }

    return editor.exit_code;
}
