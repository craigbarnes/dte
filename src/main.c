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
#include "command/cache.h"
#include "command/error.h"
#include "commands.h"
#include "compat.h"
#include "compiler.h"
#include "config.h"
#include "editor.h"
#include "encoding.h"
#include "file-history.h"
#include "frame.h"
#include "history.h"
#include "load-save.h"
#include "move.h"
#include "palette.h"
#include "search.h"
#include "showkey.h"
#include "signals.h"
#include "syntax/state.h"
#include "syntax/syntax.h"
#include "tag.h"
#include "terminal/input.h"
#include "terminal/ioctl.h"
#include "terminal/key.h"
#include "terminal/mode.h"
#include "terminal/output.h"
#include "terminal/paste.h"
#include "terminal/terminal.h"
#include "trace.h"
#include "ui.h"
#include "util/debug.h"
#include "util/exitcode.h"
#include "util/fd.h"
#include "util/log.h"
#include "util/macros.h"
#include "util/path.h"
#include "util/progname.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"
#include "util/xsnprintf.h"
#include "version.h"
#include "view.h"
#include "window.h"

static void cleanup_handler(void *userdata)
{
    EditorState *e = userdata;
    set_fatal_error_cleanup_handler(NULL, NULL);
    if (!e->child_controls_terminal) {
        ui_end(e, true);
        term_cooked();
    }
}

static ExitCode list_builtin_configs(void)
{
    String str = dump_builtin_configs();
    BUG_ON(!str.buffer);
    ExitCode e = ec_write_stdout(str.buffer, str.len);
    string_free(&str);
    return e;
}

static ExitCode dump_builtin_config(const char *name)
{
    const BuiltinConfig *cfg = get_builtin_config(name);
    if (!cfg) {
        return ec_usage_error("no built-in config with name '%s'", name);
    }
    return ec_write_stdout(cfg->text.data, cfg->text.length);
}

static ExitCode lint_syntax(const char *filename, SyntaxLoadFlags flags)
{
    EditorState *e = init_editor_state(EFLAG_HEADLESS);
    e->err.print_to_stderr = true;
    BUG_ON(e->status != EDITOR_INITIALIZING);

    int err;
    const Syntax *s = load_syntax_file(e, filename, flags | SYN_MUST_EXIST, &err);

    if (s) {
        const size_t n = s->states.count;
        const char *plural = (n == 1) ? "" : "s";
        printf("OK: loaded syntax '%s' with %zu state%s\n", s->name, n, plural);
    } else if (err == EINVAL) {
        error_msg(&e->err, "%s: no default syntax found", filename);
    }

    unsigned int nr_errs = e->err.nr_errors;
    free_editor_state(e);
    return nr_errs ? EC_DATA_ERROR : EC_OK;
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
                return ec_error("fcntl", EC_OS_ERROR);
            }
            std_fds[i] = fd;
        }

        // Ensure standard streams are connected to the terminal during
        // editor operation, regardless of how they were redirected
        if (unlikely(!freopen("/dev/tty", i ? "w" : "r", streams[i]))) {
            const char *err = strerror(errno);
            fprintf(stderr, "Failed to open /dev/tty for fd %d: %s\n", i, err);
            return EC_IO_ERROR;
        }

        int new_fd = fileno(streams[i]);
        if (unlikely(new_fd != i)) {
            // This should never happen in a single-threaded program.
            // freopen() should call fclose() followed by open() and
            // POSIX requires a successful call to open() to return the
            // lowest available file descriptor.
            fprintf(stderr, "freopen() changed fd from %d to %d\n", i, new_fd);
            return EC_OS_ERROR;
        }

        if (unlikely(!is_controlling_tty(new_fd))) {
            return ec_error("tcgetpgrp", EC_OS_ERROR);
        }
    }

    return EC_OK;
}

static ExitCode init_std_fds_headless(int std_fds[2])
{
    FILE *streams[] = {stdin, stdout};
    for (int i = 0; i < ARRAYLEN(streams); i++) {
        if (!isatty(i)) {
            // Try to duplicate redirected stdin/stdout, as described
            // in init_std_fds()
            int fd = fcntl(i, F_DUPFD_CLOEXEC, 3);
            if (fd == -1 && errno != EBADF) {
                return ec_error("fcntl", EC_OS_ERROR);
            }
            std_fds[i] = fd;
        }
        // Always reopen stdin and stdout as /dev/null, regardless of
        // whether they were duplicated above
        if (!freopen("/dev/null", i ? "w" : "r", streams[i])) {
            return ec_error("freopen", EC_IO_ERROR);
        }
    }

    if (!fd_is_valid(STDERR_FILENO)) {
        // If FD 2 isn't valid (e.g. because it was closed), point it
        // to /dev/null to ensure open(3) doesn't allocate it to other
        // opened files
        if (!freopen("/dev/null", "w", stderr)) {
            return ec_error("freopen", EC_IO_ERROR);
        }
    }

    return EC_OK;
}

// Open special Buffers for duplicated stdin and/or stdout FDs, as created
// by init_std_fds()
static Buffer *init_std_buffer(EditorState *e, int fds[2])
{
    const char *name = NULL;
    Buffer *buffer = NULL;

    if (fds[STDIN_FILENO] >= 3) {
        buffer = buffer_new(&e->buffers, &e->options, encoding_from_type(UTF8));
        if (read_blocks(buffer, fds[STDIN_FILENO], false)) {
            name = "(stdin)";
            buffer->temporary = true;
        } else {
            error_msg(&e->err, "Unable to read redirected stdin");
            buffer_remove_unlock_and_free(&e->buffers, buffer, &e->err, &e->locks_ctx);
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
        buffer_set_display_filename(buffer, xstrdup(name));
    }

    return buffer;
}

// Write the contents of the special `stdout` Buffer (as created by
// init_std_buffer()) to `stdout` and free its remaining allocations
static int buffer_write_blocks_and_free(Buffer *buffer, int fd)
{
    BUG_ON(!buffer->stdout_buffer);
    const Block *blk;
    int err = 0;

    block_for_each(blk, &buffer->blocks) {
        if (xwrite_all(fd, blk->data, blk->size) < 0) {
            err = errno;
            break;
        }
    }

    // Other allocations for `buffer` should have already been freed
    // by buffer_unlock_and_free()
    free_blocks(buffer);
    free(buffer);
    return err;
}

static void init_trace_logging(LogLevel level)
{
    if (!TRACE_LOGGING_ENABLED || level < LOG_LEVEL_TRACE) {
        return;
    }

    const char *flagstr = xgetenv("DTE_LOG_TRACE");
    if (!flagstr) {
        LOG_NOTICE("DTE_LOG_LEVEL=trace used with no effect; DTE_LOG_TRACE also needed");
        return;
    }

    TraceLoggingFlags flags = trace_flags_from_str(flagstr);
    if (!flags) {
        LOG_WARNING("no known flags in DTE_LOG_TRACE='%s'", flagstr);
        return;
    }

    LOG_INFO("DTE_LOG_TRACE=%s", flagstr);
    set_trace_logging_flags(flags);
}

static ExitCode init_logging(void)
{
    const char *filename = xgetenv("DTE_LOG");
    if (!filename) {
        return EC_OK;
    }

    const char *req_level_str = getenv("DTE_LOG_LEVEL");
    LogLevel req_level = log_level_from_str(req_level_str);
    if (req_level == LOG_LEVEL_NONE) {
        return EC_OK;
    }
    if (req_level == LOG_LEVEL_INVALID) {
        return ec_usage_error("invalid $DTE_LOG_LEVEL value: '%s'", req_level_str);
    }

    const char *no_color = xgetenv("NO_COLOR"); // https://no-color.org/
    LogLevel got_level = log_open(filename, req_level, !no_color);
    if (got_level == LOG_LEVEL_NONE) {
        const char *err = strerror(errno);
        fprintf(stderr, "Failed to open $DTE_LOG (%s): %s\n", filename, err);
        return EC_IO_ERROR;
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

    init_trace_logging(got_level);
    return EC_OK;
}

static void read_history_files(EditorState *e, bool headless)
{
    const char *dir = e->user_config_dir;
    size_t size_limit = 64u << 20; // 64 MiB
    file_history_load(&e->file_history, &e->err, path_join(dir, "file-history"), size_limit);
    history_load(&e->command_history, &e->err, path_join(dir, "command-history"), size_limit);
    history_load(&e->search_history, &e->err, path_join(dir, "search-history"), size_limit);

    // There's not much sense in saving history for headless sessions, but we
    // do still load it (above), to make it available to e.g. `exec -i command`
    if (!headless) {
        e->flags |= EFLAG_ALL_HIST;
    }

    if (e->search_history.last) {
        search_set_regexp(&e->search, e->search_history.last->text);
    }
}

static void write_history_files(EditorState *e)
{
    EditorFlags flags = e->flags;
    if (flags & EFLAG_CMD_HIST) {
        history_save(&e->command_history, &e->err);
    }
    if (flags & EFLAG_SEARCH_HIST) {
        history_save(&e->search_history, &e->err);
    }
    if (flags & EFLAG_FILE_HIST) {
        file_history_save(&e->file_history, &e->err);
    }
}

static void lookup_tags (
    EditorState *e,
    const char *tags[],
    size_t nr_tags,
    View *closeable_default_view
) {
    if (nr_tags == 0 || !load_tag_file(&e->tagfile, &e->err)) {
        return;
    }

    MessageArray *msgs = &e->messages[e->options.msg_tag];
    clear_messages(msgs);

    for (size_t i = 0; i < nr_tags; i++) {
        StringView tagname = strview_from_cstring(tags[i]);
        tag_lookup(&e->tagfile, msgs, &e->err, &tagname, NULL);
    }

    if (
        activate_current_message(msgs, e->window)
        && closeable_default_view
        && e->window->views.count >= 2
    ) {
        // Close the default/empty buffer, if another buffer is opened
        // for a tag (and no commands were executed via `-c` or `-C`)
        remove_view(closeable_default_view);
    }
}

static View *open_initial_buffers (
    EditorState *e,
    Buffer *std_buffer,
    char *args[],
    size_t len,
    bool headless
) {
    // Don't print error messages to `stderr`, unless in headless mode
    const unsigned int nerrs_save = e->err.nr_errors;
    const bool ptse_save = e->err.print_to_stderr;
    e->err.print_to_stderr = headless;

    // Open files specified as command-line arguments
    Window *window = e->window;
    for (size_t i = 0, line = 0, col = 0; i < len; i++) {
        const char *str = args[i];
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
        // Open special buffer for redirected stdin/stdout
        window_add_buffer(window, std_buffer);
    }

    // Open default buffer, if none were opened above
    View *dview = window->views.count ? NULL : window_open_empty_buffer(window);
    set_view(dview ? dview : window_get_first_view(window));

    // Restore `nr_errors` from the value saved above, so that any
    // errors not printed to stderr are also excluded from the
    // `any_key()` condition in `main()`.
    e->err.nr_errors = nerrs_save;
    e->err.print_to_stderr = ptse_save;

    return dview;
}

static const char copyright[] =
    "dte " VERSION "\n"
    "(C) 2013-2024 Craig Barnes\n"
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
    static const char optstring[] = "hBHKPRVZC:Q:S:b:c:t:r:s:";
    const char *rc = NULL;
    const char *commands[8];
    const char *tags[8];
    size_t nr_commands = 0;
    size_t nr_tags = 0;
    bool headless = false;
    bool read_rc = true;
    bool load_and_save_history = true;
    bool explicit_term_query_level = false;
    unsigned int terminal_query_level = 1;

    for (int ch; (ch = getopt(argc, argv, optstring)) != -1; ) {
        switch (ch) {
        case 'C':
            headless = true;
            // Fallthrough
        case 'c':
            if (unlikely(nr_commands >= ARRAYLEN(commands))) {
                return ec_usage_error("too many -c or -C options used");
            }
            commands[nr_commands++] = optarg;
            break;
        case 't':
            if (unlikely(nr_tags >= ARRAYLEN(tags))) {
                return ec_usage_error("too many -t options used");
            }
            tags[nr_tags++] = optarg;
            break;
        case 'Q':
            if (!str_to_uint(optarg, &terminal_query_level)) {
                return ec_usage_error("invalid argument for -Q: '%s'", optarg);
            }
            explicit_term_query_level = true;
            break;
        case 'H': load_and_save_history = false; break;
        case 'r': rc = optarg; break;
        case 'R': read_rc = false; break;
        case 'b': return dump_builtin_config(optarg);
        case 'B': return list_builtin_configs();
        case 'h': return ec_printf_ok(usage, progname(argc, argv, "dte"));
        case 'K': return showkey_loop(terminal_query_level);
        case 'P': return print_256_color_palette();
        case 's': return lint_syntax(optarg, 0);
        case 'S': return lint_syntax(optarg, SYN_LINT);
        case 'V': return ec_write_stdout(copyright, sizeof(copyright));
        case 'Z': return ec_printf_ok("features:%s\n", buildvar_string);
        default: return EC_USAGE_ERROR;
        }
    }

    // stdio(3) fds must be handled before calling init_logging(), otherwise
    // an invocation like e.g. `DTE_LOG=/dev/pts/2 dte 0<&-` could cause the
    // logging fd to be opened as STDIN_FILENO
    int std_fds[2] = {-1, -1};
    ExitCode r = headless ? init_std_fds_headless(std_fds) : init_std_fds(std_fds);
    r = r ? r : init_logging();
    if (unlikely(r != EC_OK)) {
        return r;
    }

    struct utsname u;
    if (likely(uname(&u) >= 0)) {
        LOG_INFO("system: %s/%s %s", u.sysname, u.machine, u.release);
        if (
            !headless
            && !explicit_term_query_level
            && str_has_suffix(u.release, "-WSL2")
        ) {
            // There appears to be an issue on WSL2 where the DA1 query
            // response is interpreted as pasted text and then inserted
            // into the buffer at startup. For now, we simply disable
            // querying entirely on that platform, until the problem
            // can be investigated.
            LOG_NOTICE("WSL2 detected; disabling all terminal queries");
            terminal_query_level = 0;
        }
    } else {
        LOG_ERRNO("uname");
    }

    LOG_INFO("dte version: " VERSION);
    LOG_INFO("build vars:%s", buildvar_string);

    if (headless) {
        set_signal_dispositions_headless();
    } else {
        if (!term_mode_init()) {
            return ec_error("tcgetattr", EC_IO_ERROR);
        }
        set_basic_signal_dispositions();
    }

    // Note that we set EFLAG_HEADLESS here, regardless of whether -C was used,
    // because the terminal isn't properly initialized until later and we don't
    // want commands running via -c or -C interacting with it
    EditorState *e = init_editor_state(EFLAG_HEADLESS);

    Terminal *term = &e->terminal;
    e->err.print_to_stderr = true;

    if (!headless) {
        term_init(term, getenv("TERM"), getenv("COLORTERM"));
    }

    Buffer *std_buffer = init_std_buffer(e, std_fds);
    bool have_stdout_buffer = std_buffer && std_buffer->stdout_buffer;

    // Create this early (needed if "lock-files" is true)
    const char *cfgdir = e->user_config_dir;
    BUG_ON(!cfgdir);
    if (mkdir(cfgdir, 0755) != 0 && errno != EEXIST) {
        error_msg(&e->err, "Error creating %s: %s", cfgdir, strerror(errno));
        load_and_save_history = false;
        e->options.lock_files = false;
    }

    exec_rc_files(e, rc, read_rc);

    e->window = new_window(e);
    e->root_frame = new_root_frame(e->window);

    if (load_and_save_history) {
        read_history_files(e, headless);
    }

    e->status = EDITOR_RUNNING;
    char **files = argv + optind;
    size_t nfiles = argc - optind;
    View *dview = open_initial_buffers(e, std_buffer, files, nfiles, headless);

    if (!headless) {
        update_screen_size(term, e->root_frame);
    }

    for (size_t i = 0; i < nr_commands; i++) {
        handle_normal_command(e, commands[i], false);
    }

    bool fast_exit = e->status != EDITOR_RUNNING;
    if (headless || fast_exit) {
        // Headless mode implicitly exits after commands have finished
        // running. We may also exit here (without drawing the UI,
        // emitting terminal queries or entering main_loop()) if a `-c`
        // command has already quit the editor. This prevents commands
        // like e.g. `dte -HR -cquit` from emitting terminal queries
        // and exiting before the responses have been read (which would
        // typically cause the parent shell process to read them
        // instead).
        goto exit;
    }

    lookup_tags(e, tags, nr_tags, nr_commands ? NULL : dview);

    set_fatal_error_cleanup_handler(cleanup_handler, e);
    set_fatal_signal_handlers();
    set_sigwinch_handler();

    if (unlikely(!term_raw())) {
        e->status = ec_error("tcsetattr", EC_IO_ERROR);
        fast_exit = true;
        goto exit;
    }

    if (e->err.nr_errors) {
        // Display "press any key to continue" prompt for stderr errors
        any_key(term, e->options.esc_timeout);
        clear_error(&e->err);
    }

    e->err.print_to_stderr = false;
    e->flags &= ~EFLAG_HEADLESS; // See comment for init_editor_state() call above
    ui_first_start(e, terminal_query_level);
    main_loop(e);
    term_restore_title(term);

    /*
     * This is normally followed immediately by term_cooked() in other
     * contexts, but in this case we want to switch back to cooked mode
     * as late as possible. On underpowered machines, unlocking files and
     * writing history may take some time. If cooked mode were switched
     * to here it'd leave a window of time where we've switched back to
     * the normal screen buffer and reset the termios ECHO flag while
     * still being the foreground process, which would cause any input
     * to be echoed to the terminal (before the shell takes over again
     * and prints its prompt).
     */
    ui_end(e, true);

exit:
    e->err.print_to_stderr = true;
    frame_remove(e, e->root_frame); // Unlock files and add to file history
    write_history_files(e);
    int exit_code = MAX(e->status, EDITOR_EXIT_OK);

    if (!headless && !fast_exit) {
        // This must be done before calling buffer_write_blocks_and_free(),
        // since output modes need to be restored to get proper line ending
        // translation and std_fds[STDOUT_FILENO] may be a pipe to the
        // terminal
        term_cooked();
    }

    if (have_stdout_buffer) {
        int err = buffer_write_blocks_and_free(std_buffer, std_fds[STDOUT_FILENO]);
        if (err != 0) {
            error_msg(&e->err, "failed to write (stdout) buffer: %s", strerror(err));
            if (exit_code == EDITOR_EXIT_OK) {
                exit_code = EC_IO_ERROR;
            }
        }
    }

    if (DEBUG >= 1 || ASAN_ENABLED || xgetenv(ld_preload_env_var())) {
        // Only free EditorState in debug builds or when a library/tool is
        // checking for leaks; otherwise it's pointless to do so immediately
        // before exiting
        free_editor_state(e);
    }

    LOG_INFO("exiting with status %d", exit_code);
    log_close();
    return exit_code;
}
