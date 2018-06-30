#include "editor.h"
#include "window.h"
#include "view.h"
#include "frame.h"
#include "term-info.h"
#include "term-read.h"
#include "term-write.h"
#include "config.h"
#include "color.h"
#include "syntax.h"
#include "alias.h"
#include "history.h"
#include "file-history.h"
#include "search.h"
#include "error.h"
#include "move.h"
#include "screen.h"

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
        resize();
    }
}

static void handle_fatal_signal(int signum)
{
    term_cleanup();

    set_signal_handler(signum, SIG_DFL);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signum);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    raise(signum);
}

static inline void do_sigaction(int sig, const struct sigaction *action)
{
    if (sigaction(sig, action, NULL) != 0) {
        BUG("Failed to add handler for signal %d: %s", sig, strerror(errno));
    }
}

static void set_signal_handlers(void)
{
    // SIGABRT is not included here, since we can always call
    // term_cleanup() explicitly, before calling abort().
    static const int fatal_signals[] = {
        SIGBUS, SIGFPE, SIGILL, SIGSEGV,
        SIGSYS, SIGTRAP, SIGXCPU, SIGXFSZ,
        SIGALRM, SIGPROF, SIGVTALRM,
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
    memzero(&action);
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

static int dump_builtin_config(const char *const name)
{
    const BuiltinConfig *cfg = get_builtin_config(name);
    if (cfg) {
        fputs(cfg->text, stdout);
        return 0;
    } else {
        fprintf(stderr, "Error: no built-in config with name '%s'\n", name);
        return 1;
    }
}

static void showkey_loop(void)
{
    term_raw();
    if (terminal.control_codes->init) {
        fputs(terminal.control_codes->init, stdout);
    }
    if (terminal.control_codes->keypad_on) {
        fputs(terminal.control_codes->keypad_on, stdout);
    }
    puts("\nPress any key combination, or use Ctrl+D to exit\n");
    fflush(stdout);

    bool loop = true;
    while (loop) {
        Key key;
        if (!term_read_key(&key)) {
            const char *seq = term_get_last_key_escape_sequence();
            if (seq) {
                printf("   %-12s %-14s ^[%s\n", "UNKNOWN", "-", seq);
                fflush(stdout);
            }
            continue;
        }
        switch (key) {
        case KEY_PASTE:
            term_discard_paste();
            continue;
        case CTRL('D'):
            loop = false;
            break;
        }
        char *str = key_to_string(key);
        const char *seq = term_get_last_key_escape_sequence();
        printf("   %-12s 0x%-12" PRIX32 " %s\n", str, key, seq ? seq : "");
        free(str);
        fflush(stdout);
    }

    if (terminal.control_codes->keypad_off) {
        fputs(terminal.control_codes->keypad_off, stdout);
    }
    if (terminal.control_codes->deinit) {
        fputs(terminal.control_codes->deinit, stdout);
    }
    fflush(stdout);
    term_cooked();
}

static const char usage[] =
    "Usage: %s [OPTIONS] [[+LINE] FILE]...\n\n"
    "Options:\n"
    "   -c COMMAND  Run COMMAND after editor starts\n"
    "   -t CTAG     Jump to source location of CTAG\n"
    "   -r RCFILE   Read user config from RCFILE instead of ~/.dte/rc\n"
    "   -b NAME     Print built-in config matching NAME and exit\n"
    "   -B          Print list of built-in config names and exit\n"
    "   -R          Don't read user config file\n"
    "   -K          Start editor in \"showkey\" mode\n"
    "   -h          Display help summary and exit\n"
    "   -V          Display version number and exit\n"
    "\n";

int main(int argc, char *argv[])
{
    static const char optstring[] = "hBHKRVb:c:t:r:";
    const char *tag = NULL;
    const char *rc = NULL;
    const char *command = NULL;
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
            break;
        case 'V':
            printf("dte %s\n", editor.version);
            puts("(C) 2017-2018 Craig Barnes");
            puts("(C) 2010-2015 Timo Hirvonen");
            return 0;
        case 'h':
            printf(usage, argv[0]);
            return 0;
        case '?':
        default:
            return 1;
        }
    }

    if (!isatty(STDOUT_FILENO)) {
        fputs("stdout doesn't refer to a terminal\n", stderr);
        return 1;
    }
    if (!isatty(STDIN_FILENO)) {
        if (!freopen("/dev/tty", "r", stdin)) {
            fputs("Cannot reopen input tty\n", stderr);
            return 1;
        }
    }

    term_init();

    if (use_showkey) {
        showkey_loop();
        return 0;
    }

    // Create this early. Needed if lock-files is true.
    const char *const editor_dir = editor.user_config_dir;
    if (mkdir(editor_dir, 0755) != 0 && errno != EEXIST) {
        error_msg("Error creating %s: %s", editor_dir, strerror(errno));
    }

    save_term_title();
    exec_reset_colors_rc();
    read_config(commands, "rc", CFG_MUST_EXIST | CFG_BUILTIN);
    fill_builtin_colors();

    // NOTE: syntax_changed() uses window. Should possibly create
    // window after reading rc.
    window = new_window();
    root_frame = new_root_frame(window);

    if (read_rc) {
        if (rc) {
            read_config(commands, rc, CFG_MUST_EXIST);
        } else {
            char *filename = editor_file("rc");
            read_config(commands, filename, CFG_NOFLAGS);
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
    if (nr_errors) {
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

    View *empty_buffer = NULL;
    if (window->views.count == 0) {
        empty_buffer = window_open_empty_buffer(window);
    }

    set_view(window->views.ptrs[0]);

    if (command || tag) {
        resize();
    }

    if (command) {
        handle_command(commands, command);
    }

    if (tag) {
        PointerArray array = PTR_ARRAY_INIT;
        ptr_array_add(&array, xstrdup("tag"));
        ptr_array_add(&array, xstrdup(tag));
        ptr_array_add(&array, NULL);
        run_commands(commands, &array);
        ptr_array_free(&array);
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

    if (terminal.control_codes->init) {
        buf_escape(terminal.control_codes->init);
    }

    resize();
    main_loop();
    restore_term_title();
    ui_end();

    if (terminal.control_codes->deinit) {
        buf_escape(terminal.control_codes->deinit);
        buf_flush();
    }

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

    return 0;
}
