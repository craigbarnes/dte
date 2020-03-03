#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "alias.h"
#include "block.h"
#include "config.h"
#include "debug.h"
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
#include "util/macros.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"
#include "view.h"
#include "window.h"

// Semantic exit codes, as defined by BSD sysexits(3)
enum {
    EX_USAGE = 64, // Command line usage error
    EX_DATAERR = 65, // Input data error
    EX_OSERR = 71, // Operating system error
    EX_IOERR = 74, // Input/output error
};

static void handle_sigtstp(int UNUSED_ARG(signum))
{
    suspend();
}

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

static noreturn COLD void handle_fatal_signal(int signum)
{
    term_cleanup();

    struct sigaction sa;
    MEMZERO(&sa);
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = SIG_DFL;
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
// * SIGURG, SIGCHLD (already ignored by default)
// * SIGKILL, SIGSTOP (can't be caught or ignored)
// * SIGPOLL, SIGPROF (marked "obsolete" in POSIX 2008)
// * SIGABRT (cleanup is done before calling abort())
static void set_signal_handlers(void)
{
    static const int fatal_signals[] = {
        SIGBUS, SIGFPE, SIGILL, SIGSEGV,
        SIGSYS, SIGTRAP, SIGXCPU, SIGXFSZ,
        SIGALRM, SIGVTALRM,
        SIGHUP, SIGTERM,
    };

    static const int ignored_signals[] = {
        SIGINT, SIGQUIT, SIGPIPE,
        SIGUSR1, SIGUSR2,
    };

    static const struct {
        int signum;
        void (*handler)(int);
    } handled_signals[] = {
        {SIGTSTP, handle_sigtstp},
        {SIGCONT, handle_sigcont},
#ifdef SIGWINCH
        {SIGWINCH, handle_sigwinch},
#endif
    };

    struct sigaction action;
    MEMZERO(&action);
    sigfillset(&action.sa_mask);

    action.sa_handler = handle_fatal_signal;
    for (size_t i = 0; i < ARRAY_COUNT(fatal_signals); i++) {
        do_sigaction(fatal_signals[i], &action);
    }

    action.sa_handler = SIG_IGN;
    for (size_t i = 0; i < ARRAY_COUNT(ignored_signals); i++) {
        do_sigaction(ignored_signals[i], &action);
    }

    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < ARRAY_COUNT(handled_signals); i++) {
        action.sa_handler = handled_signals[i].handler;
        do_sigaction(handled_signals[i].signum, &action);
    }
}

static int dump_builtin_config(const char *name)
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
    return 0;
}

static void showkey_loop(void)
{
    term_raw();
    terminal.put_control_code(terminal.control_codes.init);
    terminal.put_control_code(terminal.control_codes.keypad_on);
    term_add_literal("Press any key combination, or use Ctrl+D to exit\r\n");
    term_output_flush();

    bool loop = true;
    while (loop) {
        KeyCode key;
        const char *str;
        if (!term_read_key(&key)) {
            str = "UNKNOWN";
        } else if (key == KEY_PASTE) {
            term_discard_paste();
            continue;
        } else {
            if (key == CTRL('D')) {
                loop = false;
            }
            str = keycode_to_string(key);
        }
        term_add_literal("  ");
        term_add_bytes(str, strlen(str));
        term_add_literal("\r\n");
        term_output_flush();
    }

    terminal.put_control_code(terminal.control_codes.keypad_off);
    terminal.put_control_code(terminal.control_codes.deinit);
    term_output_flush();
    term_cooked();
}

static const char copyright[] =
    "(C) 2017-2020 Craig Barnes\n"
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
    const char *lint_syntax = NULL;
    bool read_rc = true;
    bool use_showkey = false;
    bool load_and_save_history = true;
    int ch;

    init_editor_state();

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
            lint_syntax = optarg;
            goto loop_break;
        case 'R':
            read_rc = false;
            break;
        case 'b':
            return dump_builtin_config(optarg);
        case 'B':
            list_builtin_configs();
            return 0;
        case 'H':
            load_and_save_history = false;
            break;
        case 'K':
            use_showkey = true;
            goto loop_break;
        case 'V':
            printf("dte %s\n", editor.version);
            puts(copyright);
            return 0;
        case 'h':
            printf(usage, argv[0]);
            return 0;
        case '?':
        default:
            return EX_USAGE;
        }
    }

loop_break:

    if (lint_syntax) {
        int err;
        const Syntax *s = load_syntax_file(lint_syntax, CFG_MUST_EXIST, &err);
        if (s) {
            const size_t n = s->states.count;
            const char *p = (n > 1) ? "s" : "";
            printf("OK: loaded syntax '%s' with %zu state%s\n", s->name, n, p);
        } else if (err == EINVAL) {
            error_msg("%s: no default syntax found", lint_syntax);
        }
        return get_nr_errors() ? EX_DATAERR : 0;
    }

    Buffer *stdin_buffer = NULL;
    if (!isatty(STDIN_FILENO)) {
        Buffer *b = buffer_new(&editor.charset);
        if (read_blocks(b, STDIN_FILENO) == 0) {
            set_display_filename(b, xmemdup_literal("(stdin)"));
            stdin_buffer = b;
            stdin_buffer->temporary = true;
        } else {
            if (errno != EBADF) {
                error_msg("Unable to read redirected stdin");
            }
            free_buffer(b);
        }
        if (!freopen("/dev/tty", "r", stdin)) {
            perror("Unable to reopen input tty");
            return EX_IOERR;
        }
        int fd = fileno(stdin);
        if (fd != STDIN_FILENO) {
            fprintf(stderr, "freopen() changed stdin fd from 0 to %d\n", fd);
            return EX_OSERR;
        }
    }

    Buffer *stdout_buffer = NULL;
    int old_stdout_fd = -1;
    if (!isatty(STDOUT_FILENO)) {
        old_stdout_fd = fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3);
        if (old_stdout_fd == -1 && errno != EBADF) {
            perror("fcntl");
            return EX_OSERR;
        }
        if (!freopen("/dev/tty", "w", stdout)) {
            perror("Unable to open /dev/tty");
            return EX_OSERR;
        }
        int fd = fileno(stdout);
        if (fd != STDOUT_FILENO) {
            // This should never happen in a single-threaded program.
            // freopen() should call fclose() followed by open() and
            // POSIX requires a successful call to open() to return the
            // lowest available file descriptor.
            fprintf(stderr, "freopen() changed stdout fd from 1 to %d\n", fd);
            return EX_OSERR;
        }
        if (old_stdout_fd == -1) {
            // The call to fcntl(3) above failed with EBADF, meaning stdout was
            // most likely closed and there's no point opening a buffer for it
        } else if (stdin_buffer) {
            stdout_buffer = stdin_buffer;
            stdout_buffer->stdout_buffer = true;
            set_display_filename(stdout_buffer, xmemdup_literal("(stdin|stdout)"));
        } else {
            stdout_buffer = open_empty_buffer("(stdout)");
            stdout_buffer->stdout_buffer = true;
            stdout_buffer->temporary = true;
        }
    }

    term_init();

    if (use_showkey) {
        showkey_loop();
        return 0;
    }

    // Create this early. Needed if lock-files is true.
    const char *editor_dir = editor.user_config_dir;
    if (mkdir(editor_dir, 0755) != 0 && errno != EEXIST) {
        error_msg("Error creating %s: %s", editor_dir, strerror(errno));
        load_and_save_history = false;
        editor.options.lock_files = false;
    }

    terminal.save_title();
    exec_builtin_rc();

    // NOTE: syntax_changed() uses window. Should possibly create
    // window after reading rc.
    window = new_window();
    root_frame = new_root_frame(window);

    if (read_rc) {
        if (rc) {
            read_config(&commands, rc, CFG_MUST_EXIST);
        } else {
            char *filename = editor_file("rc");
            read_config(&commands, filename, CFG_NOFLAGS);
            free(filename);
        }
    }

    update_all_syntax_colors();
    sort_aliases();

    set_signal_handlers();

    char *file_history_filename = NULL;
    char *command_history_filename = NULL;
    char *search_history_filename = NULL;
    if (load_and_save_history) {
        file_history_filename = editor_file("file-history");
        command_history_filename = editor_file("command-history");
        search_history_filename = editor_file("search-history");

        load_file_history(file_history_filename);

        history_load (
            &editor.command_history,
            command_history_filename,
            command_history_size
        );

        history_load (
            &editor.search_history,
            search_history_filename,
            search_history_size
        );
        if (editor.search_history.count) {
            search_set_regexp (
                editor.search_history.ptrs[editor.search_history.count - 1]
            );
        }
    }

    // Initialize terminal but don't update screen yet. Also display
    // "Press any key to continue" prompt if there were any errors
    // during reading configuration files.
    term_raw();
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
        handle_command(&commands, command, false);
    }

    if (tag) {
        String s = STRING_INIT;
        string_append_literal(&s, "tag ");
        string_append_escaped_arg(&s, tag, true);
        handle_command(&commands, string_borrow_cstring(&s), false);
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

    terminal.restore_title();
    ui_end();
    term_output_flush();

    // Unlock files and add files to file history
    remove_frame(root_frame);

    if (load_and_save_history) {
        history_save(&editor.command_history, command_history_filename);
        free(command_history_filename);

        history_save(&editor.search_history, search_history_filename);
        free(search_history_filename);

        save_file_history(file_history_filename);
        free(file_history_filename);
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

    return 0;
}
