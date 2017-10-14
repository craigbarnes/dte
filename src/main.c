#include "editor.h"
#include "window.h"
#include "view.h"
#include "frame.h"
#include "term.h"
#include "config.h"
#include "color.h"
#include "syntax.h"
#include "alias.h"
#include "history.h"
#include "file-history.h"
#include "search.h"
#include "error.h"
#include "move.h"
#include "BUILTIN_CONFIG.h"

static void handle_sigtstp(int signum)
{
    suspend();
}

static void handle_sigcont(int signum)
{
    if (
        !editor.child_controls_terminal
        && editor.status != EDITOR_INITIALIZING
    ) {
        term_raw();
        resize();
    }
}

static void handle_sigwinch(int signum)
{
    editor.resized = true;
}

int main(int argc, char *argv[])
{
    static const char *const opts = "[-RV] [-c command] [-t tag] [-r rcfile] [[+line] file]...";
    static const char *const optstring = "RVc:t:r:";
    const char *const term = getenv("TERM");
    const char *tag = NULL;
    const char *rc = NULL;
    const char *command = NULL;
    char *command_history_filename;
    char *search_history_filename;
    char *editor_dir;
    bool read_rc = true;
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
        case 'V':
            printf("dte %s\n", editor.version);
            puts("(C) 2017 Craig Barnes");
            puts("(C) 2010-2015 Timo Hirvonen");
            return 0;
        case '?':
        default:
            fprintf(stderr, "Usage: %s %s\n", argv[0], opts);
            return 1;
        }
    }

    if (!isatty(1)) {
        fputs("stdout doesn't refer to a terminal\n", stderr);
        return 1;
    }
    if (term == NULL || term[0] == 0) {
        fputs("TERM not set\n", stderr);
        return 1;
    }
    term_init(term);

    // Create this early. Needed if lock-files is true.
    editor_dir = editor_file("");
    mkdir(editor_dir, 0755);
    free(editor_dir);

    exec_builtin_rc("hi\n");
    exec_builtin_rc(builtin_config);
    fill_builtin_colors();

    // NOTE: syntax_changed() uses window. Should possibly create
    // window after reading rc.
    window = new_window();
    root_frame = new_root_frame(window);

    if (read_rc) {
        if (rc) {
            read_config(commands, rc, true);
        } else {
            char *filename = editor_file("rc");
            read_config(commands, filename, false);
            free(filename);
        }
    }

    update_all_syntax_colors();
    sort_aliases();

    // Terminal does not generate signals for control keys
    set_signal_handler(SIGINT, SIG_IGN);
    set_signal_handler(SIGQUIT, SIG_IGN);
    set_signal_handler(SIGPIPE, SIG_IGN);

    // Terminal does not generate signal for ^Z but someone can send
    // us SIGTSTP nevertheless. SIGSTOP can't be caught.
    set_signal_handler(SIGTSTP, handle_sigtstp);

    set_signal_handler(SIGCONT, handle_sigcont);
    set_signal_handler(SIGWINCH, handle_sigwinch);

    load_file_history();
    command_history_filename = editor_file("command-history");
    search_history_filename = editor_file("search-history");
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

    if (window->views.count == 0) {
        window_open_empty_buffer(window);
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
    resize();
    main_loop();
    ui_end();

    // Unlock files and add files to file history
    remove_frame(root_frame);

    history_save(&editor.command_history, command_history_filename);
    history_save(&editor.search_history, search_history_filename);
    free(command_history_filename);
    free(search_history_filename);
    save_file_history();
    return 0;
}
