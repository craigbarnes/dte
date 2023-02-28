#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "block.h"
#include "commands.h"
#include "compiler.h"
#include "config.h"
#include "editor.h"
#include "error.h"
#include "file-history.h"
#include "frame.h"
#include "history.h"
#include "load-save.h"
#include "move.h"
#include "screen.h"
#include "search.h"
#include "signals.h"
#include "syntax/state.h"
#include "syntax/syntax.h"
#include "tag.h"
#include "terminal/input.h"
#include "terminal/key.h"
#include "terminal/mode.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "util/debug.h"
#include "util/exitcode.h"
#include "util/fd.h"
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

static ExitCode write_stdout(const char *str, size_t len)
{
    if (xwrite_all(STDOUT_FILENO, str, len) < 0) {
        perror("write");
        return EX_IOERR;
    }
    return EX_OK;
}

static ExitCode list_builtin_configs(void)
{
    String str = dump_builtin_configs();
    BUG_ON(!str.buffer);
    ExitCode e = write_stdout(str.buffer, str.len);
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
    return write_stdout(cfg->text.data, cfg->text.length);
}

static ExitCode lint_syntax(const char *filename)
{
    EditorState *e = init_editor_state();
    int err;
    BUG_ON(e->status != EDITOR_INITIALIZING);
    const Syntax *s = load_syntax_file(e, filename, CFG_MUST_EXIST, &err);
    if (s) {
        const size_t n = s->states.count;
        const char *plural = (n == 1) ? "" : "s";
        printf("OK: loaded syntax '%s' with %zu state%s\n", s->name, n, plural);
    } else if (err == EINVAL) {
        error_msg("%s: no default syntax found", filename);
    }
    free_editor_state(e);
    return get_nr_errors() ? EX_DATAERR : EX_OK;
}

static ExitCode showkey_loop(const char *term_name, const char *colorterm)
{
    if (unlikely(!term_raw())) {
        perror("tcsetattr");
        return EX_IOERR;
    }

    Terminal term;
    TermOutputBuffer *obuf = &term.obuf;
    TermInputBuffer *ibuf = &term.ibuf;
    term_init(&term, term_name, colorterm);
    term_input_init(ibuf);
    term_output_init(obuf);
    term_enable_private_modes(&term);
    term_add_literal(obuf, "Press any key combination, or use Ctrl+D to exit\r\n");
    term_output_flush(obuf);

    char keystr[KEYCODE_STR_MAX];
    for (bool loop = true; loop; ) {
        KeyCode key = term_read_key(&term, 100);
        switch (key) {
        case KEY_NONE:
        case KEY_IGNORE:
            continue;
        case KEY_BRACKETED_PASTE:
        case KEY_DETECTED_PASTE:
            term_discard_paste(ibuf, key == KEY_BRACKETED_PASTE);
            continue;
        case MOD_CTRL | 'd':
            loop = false;
        }
        size_t keylen = keycode_to_string(key, keystr);
        term_add_literal(obuf, "  ");
        term_add_bytes(obuf, keystr, keylen);
        term_add_literal(obuf, "\r\n");
        term_output_flush(obuf);
    }

    term_restore_private_modes(&term);
    term_output_flush(obuf);
    term_cooked();
    term_input_free(ibuf);
    term_output_free(obuf);
    return EX_OK;
}

static ExitCode init_std_fds(int std_fds[2])
{
    FILE *streams[3] = {stdin, stdout, stderr};
    for (int i = 0; i < ARRAYLEN(streams); i++) {
        if (is_controlling_tty(i)) {
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

        if (unlikely(!is_controlling_tty(new_fd))) {
            perror("tcgetpgrp");
            return EX_OSERR;
        }
    }

    return EX_OK;
}

static Buffer *init_std_buffer(EditorState *e, int fds[2])
{
    const char *name = NULL;
    Buffer *buffer = NULL;

    if (fds[STDIN_FILENO] >= 3) {
        Encoding enc = encoding_from_type(UTF8);
        buffer = buffer_new(&e->buffers, &e->options, &enc);
        if (read_blocks(buffer, fds[STDIN_FILENO], false)) {
            name = "(stdin)";
            buffer->temporary = true;
        } else {
            error_msg("Unable to read redirected stdin");
            remove_and_free_buffer(&e->buffers, buffer);
            buffer = NULL;
        }
    }

    if (fds[STDOUT_FILENO] >= 3) {
        if (!buffer) {
            buffer = open_empty_buffer(&e->buffers, &e->options);
            name = "(stdout)";
        } else {
            name = "(stdin|stdout)";
        }
        buffer->stdout_buffer = true;
        buffer->temporary = true;
    }

    BUG_ON(!buffer != !name);
    if (name) {
        set_display_filename(buffer, xstrdup(name));
    }

    return buffer;
}

static ExitCode init_logging(const char *filename, const char *req_level_str)
{
    if (!filename || filename[0] == '\0') {
        return EX_OK;
    }

    LogLevel req_level = log_level_from_str(req_level_str);
    if (req_level == LOG_LEVEL_NONE) {
        return EX_OK;
    }
    if (req_level == LOG_LEVEL_INVALID) {
        fprintf(stderr, "Invalid $DTE_LOG_LEVEL value: '%s'\n", req_level_str);
        return EX_USAGE;
    }

    // https://no-color.org/
    const char *no_color = xgetenv("NO_COLOR");

    LogLevel got_level = log_open(filename, req_level, !no_color);
    if (got_level == LOG_LEVEL_NONE) {
        const char *err = strerror(errno);
        fprintf(stderr, "Failed to open $DTE_LOG (%s): %s\n", filename, err);
        return EX_IOERR;
    }

    const char *got_level_str = log_level_to_str(got_level);
    if (got_level != req_level) {
        const char *r = req_level_str;
        const char *g = got_level_str;
        LOG_WARNING("log level '%s' unavailable; falling back to '%s'", r, g);
    }

    LOG_INFO("logging to '%s' (level: %s)", filename, got_level_str);

    if (no_color) {
        LOG_INFO("log colors disabled ($NO_COLOR)");
    }

    struct utsname u;
    if (likely(uname(&u) >= 0)) {
        LOG_INFO("system: %s/%s %s", u.sysname, u.machine, u.release);
    } else {
        LOG_ERRNO("uname");
    }
    return EX_OK;
}

static void log_config_counts(const EditorState *e)
{
    if (!log_level_enabled(LOG_LEVEL_INFO)) {
        return;
    }

    size_t nbinds = 0;
    for (size_t i = 0; i < ARRAYLEN(e->modes); i++) {
        nbinds += e->modes[i].key_bindings.count;
    }

    size_t nerrorfmts = 0;
    for (HashMapIter it = hashmap_iter(&e->compilers); hashmap_next(&it); ) {
        const Compiler *compiler = it.entry->value;
        nerrorfmts += compiler->error_formats.count;
    }

    LOG_INFO (
        "binds=%zu aliases=%zu hi=%zu ft=%zu option=%zu errorfmt=%zu(%zu)",
        nbinds,
        e->aliases.count,
        e->colors.other.count + NR_BC,
        e->filetypes.count,
        e->file_options.count,
        e->compilers.count,
        nerrorfmts
    );
}

static const char copyright[] =
    "dte " VERSION "\n"
    "(C) 2013-2023 Craig Barnes\n"
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
    const char *commands[8];
    size_t nr_commands = 0;
    bool read_rc = true;
    bool use_showkey = false;
    bool load_and_save_history = true;
    set_print_errors_to_stderr(true);

    for (int ch; (ch = getopt(argc, argv, optstring)) != -1; ) {
        switch (ch) {
        case 'c':
            if (unlikely(nr_commands >= ARRAYLEN(commands))) {
                fputs("Error: too many -c options used\n", stderr);
                return EX_USAGE;
            }
            commands[nr_commands++] = optarg;
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
            return write_stdout(copyright, sizeof(copyright));
        case 'h':
            printf(usage, (argv[0] && argv[0][0]) ? argv[0] : "dte");
            return EX_OK;
        default:
            return EX_USAGE;
        }
    }

loop_break:;

    const char *term_name = xgetenv("TERM");
    if (!term_name) {
        fputs("Error: $TERM not set\n", stderr);
        // This is considered a "usage" error, because the program
        // must be started from a properly configured terminal
        return EX_USAGE;
    }

    // This must be done before calling init_logging(), otherwise an
    // invocation like e.g. `DTE_LOG=/dev/pts/2 dte 0<&-` could
    // cause the logging fd to be opened as STDIN_FILENO
    int std_fds[2] = {-1, -1};
    ExitCode r = init_std_fds(std_fds);
    if (unlikely(r != EX_OK)) {
        return r;
    }

    r = init_logging(getenv("DTE_LOG"), getenv("DTE_LOG_LEVEL"));
    if (unlikely(r != EX_OK)) {
        return r;
    }

    if (!term_mode_init()) {
        perror("tcgetattr");
        return EX_IOERR;
    }

    const char *colorterm = getenv("COLORTERM");
    if (use_showkey) {
        return showkey_loop(term_name, colorterm);
    }

    EditorState *e = init_editor_state();
    Terminal *term = &e->terminal;
    term_init(term, term_name, colorterm);

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
    exec_builtin_rc(e);

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
        read_normal_config(e, rc, flags);
    }

    log_config_counts(e);
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
        if (line == 0 && *str == '+' && str_to_filepos(str + 1, &line, &col)) {
            continue;
        }
        View *view = window_open_buffer(window, str, false, NULL);
        if (line == 0) {
            continue;
        }
        set_view(view);
        move_to_filepos(view, line, col);
        line = 0;
    }

    if (std_buffer) {
        window_add_buffer(window, std_buffer);
    }

    View *dview = NULL;
    if (window->views.count == 0) {
        // Open a default buffer, if none were opened for arguments
        dview = window_open_empty_buffer(window);
        BUG_ON(!dview);
        BUG_ON(window->views.count != 1);
        BUG_ON(dview != window->views.ptrs[0]);
    }

    set_view(window->views.ptrs[0]);
    ui_start(e);

    for (size_t i = 0; i < nr_commands; i++) {
        handle_normal_command(e, commands[i], false);
    }

    if (tag) {
        StringView tag_sv = strview_from_cstring(tag);
        if (tag_lookup(&e->tagfile, &tag_sv, NULL, &e->messages)) {
            activate_current_message(e);
            if (dview && nr_commands == 0 && window->views.count > 1) {
                // Close default/empty buffer, if `-t` jumped to a tag
                // and no commands were executed via `-c`
                remove_view(dview);
                dview = NULL;
            }
        }
    }

    if (nr_commands > 0 || tag) {
        normal_update(e);
    }

    int exit_code = main_loop(e);

    term_restore_title(term);
    ui_end(e);
    term_output_flush(&term->obuf);
    set_print_errors_to_stderr(true);

    // Unlock files and add to file history
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
                error_msg_errno("failed to write (stdout) buffer");
                if (exit_code == EDITOR_EXIT_OK) {
                    exit_code = EX_IOERR;
                }
                break;
            }
        }
        free_blocks(std_buffer);
        free(std_buffer);
    }

    free_editor_state(e);
    LOG_INFO("exiting with status %d", exit_code);
    log_close();
    return exit_code;
}
