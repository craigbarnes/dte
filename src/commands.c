#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "commands.h"
#include "bind.h"
#include "bookmark.h"
#include "buffer.h"
#include "case.h"
#include "change.h"
#include "cmdline.h"
#include "command/alias.h"
#include "command/args.h"
#include "command/macro.h"
#include "compiler.h"
#include "config.h"
#include "convert.h"
#include "copy.h"
#include "delete.h"
#include "editor.h"
#include "encoding.h"
#include "error.h"
#include "exec.h"
#include "file-option.h"
#include "filetype.h"
#include "frame.h"
#include "history.h"
#include "insert.h"
#include "join.h"
#include "load-save.h"
#include "lock.h"
#include "mode.h"
#include "move.h"
#include "msg.h"
#include "regexp.h"
#include "replace.h"
#include "search.h"
#include "selection.h"
#include "shift.h"
#include "show.h"
#include "spawn.h"
#include "syntax/color.h"
#include "syntax/syntax.h"
#include "tag.h"
#include "terminal/cursor.h"
#include "terminal/mode.h"
#include "terminal/osc52.h"
#include "terminal/style.h"
#include "terminal/terminal.h"
#include "ui.h"
#include "util/arith.h"
#include "util/array.h"
#include "util/ascii.h"
#include "util/bit.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/log.h"
#include "util/path.h"
#include "util/str-array.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/time-util.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "view.h"
#include "window.h"
#include "wrap.h"

NOINLINE
static void do_selection_noinline(View *view, SelectionType sel)
{
    // Should only be called from do_selection()
    BUG_ON(sel == view->selection);

    if (sel == SELECT_NONE) {
        unselect(view);
        return;
    }

    if (view->selection) {
        if (view->selection != sel) {
            view->selection = sel;
            // TODO: be less brute force about this; only the first/last
            // line of the selection can change in this case
            mark_all_lines_changed(view->buffer);
        }
        return;
    }

    view->sel_so = block_iter_get_offset(&view->cursor);
    view->sel_eo = SEL_EO_RECALC;
    view->selection = sel;

    // Need to mark current line changed because cursor might
    // move up or down before screen is updated
    view_update_cursor_y(view);
    buffer_mark_lines_changed(view->buffer, view->cy, view->cy);
}

static void do_selection(View *view, SelectionType sel)
{
    if (likely(sel == view->selection)) {
        // If `sel` is SELECT_NONE here, it's always equal to select_mode
        BUG_ON(!sel && view->select_mode);
        return;
    }

    do_selection_noinline(view, sel);
}

static char last_flag_or_default(const CommandArgs *a, char def)
{
    size_t n = a->nr_flags;
    return n ? a->flags[n - 1] : def;
}

static char last_flag(const CommandArgs *a)
{
    return last_flag_or_default(a, 0);
}

static bool has_flag(const CommandArgs *a, unsigned char flag)
{
    return cmdargs_has_flag(a, flag);
}

static void handle_selection_flags(View *view, const CommandArgs *a)
{
    SelectionType sel;
    if (has_flag(a, 'l')) {
        sel = SELECT_LINES;
    } else if (has_flag(a, 'c')) {
        static_assert(SELECT_CHARS < SELECT_LINES);
        sel = MAX(SELECT_CHARS, view->select_mode);
    } else {
        sel = view->select_mode;
    }
    do_selection(view, sel);
}

static bool cmd_alias(EditorState *e, const CommandArgs *a)
{
    const char *const name = a->args[0];
    const char *const cmd = a->args[1];

    if (unlikely(name[0] == '\0')) {
        return error_msg(&e->err, "Empty alias name not allowed");
    }
    if (unlikely(name[0] == '-')) {
        // Disallowing this simplifies auto-completion for "alias "
        return error_msg(&e->err, "Alias name cannot begin with '-'");
    }

    for (size_t i = 0; name[i]; i++) {
        unsigned char c = name[i];
        if (unlikely(!(is_word_byte(c) || c == '-' || c == '?' || c == '!'))) {
            return error_msg(&e->err, "Invalid byte in alias name: %c (0x%02hhX)", c, c);
        }
    }

    if (unlikely(find_normal_command(name))) {
        return error_msg(&e->err, "Can't replace existing command %s with an alias", name);
    }

    if (likely(cmd)) {
        add_alias(&e->aliases, name, cmd);
    } else {
        remove_alias(&e->aliases, name);
    }

    return true;
}

static bool cmd_bind(EditorState *e, const CommandArgs *a)
{
    const char *keystr = a->args[a->nr_flag_args];
    const char *cmd = a->args[a->nr_flag_args + 1];
    KeyCode key = parse_key_string(keystr);
    if (unlikely(key == KEY_NONE)) {
        if (has_flag(a, 'q')) {
            LOG_INFO("bind -q: dropped invalid key string: %s", keystr);
            return false;
        }
        return error_msg(&e->err, "invalid key string: %s", keystr);
    }

    ModeHandler *modes[10];
    size_t nmodes = 0;
    static_assert(ARRAYLEN(modes) <= ARRAYLEN(a->flags));
    if (has_flag(a, 'n')) {
        modes[nmodes++] = e->normal_mode;
    }
    if (has_flag(a, 'c')) {
        modes[nmodes++] = e->command_mode;
    }
    if (has_flag(a, 's')) {
        modes[nmodes++] = e->search_mode;
    }

    if (unlikely(nmodes + a->nr_flag_args > ARRAYLEN(modes))) {
        // This is already prevented by the ARGERR_TOO_MANY_OPTIONS check
        // in do_parse_args(), but since that's only incidental, it's still
        // checked here
        return error_msg(&e->err, "too many modes specified");
    }

    // Gather pointers to modes specified via `-T modename`. This is done
    // separately from adding/removing bindings, partly so that either all
    // modes are processed or none are (if the error below is triggered).
    for (size_t i = 0, n = a->nr_flag_args; i < n; i++) {
        const char *name = a->args[i];
        ModeHandler *mode = get_mode_handler(&e->modes, name);
        if (unlikely(!mode)) {
            return error_msg(&e->err, "can't bind key in unknown mode '%s'", name);
        }
        modes[nmodes++] = mode;
    }

    if (nmodes == 0) {
        // No [-cnsT] flags used; default to normal mode
        modes[nmodes++] = e->normal_mode;
    }

    if (!cmd) {
        for (size_t i = 0; i < nmodes; i++) {
            remove_binding(&modes[i]->key_bindings, key);
        }
        return true;
    }

    CommandRunner runner = cmdrunner(e, NULL);
    for (size_t i = 0; i < nmodes; i++) {
        runner.cmds = modes[i]->cmds;
        CachedCommand *cc = cached_command_new(&runner, cmd);
        add_binding(&modes[i]->key_bindings, key, cc);
    }

    return true;
}

static bool cmd_bof(EditorState *e, const CommandArgs *a)
{
    handle_selection_flags(e->view, a);
    move_bof(e->view);
    return true;
}

static bool cmd_bol(EditorState *e, const CommandArgs *a)
{
    const uint_least64_t flagset =
        cmdargs_flagset_value('r')
        | cmdargs_flagset_value('s')
        | cmdargs_flagset_value('t')
    ;

    SmartBolType type = BOL_SIMPLE;
    switch (cmdargs_pick_winning_flag_from_set(a, flagset)) {
        case 'r': type = BOL_TOGGLE_LR; break;
        case 's': type = BOL_INDENT; break;
        case 't': type = BOL_TOGGLE_RL; break;
        case 0: break;
        default: BUG("unhandled flag");
    }

    handle_selection_flags(e->view, a);
    move_bol_smart(e->view, type);
    return true;
}

static bool cmd_bolsf(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    View *view = e->view;
    handle_selection_flags(view, a);

    if (!block_iter_bol(&view->cursor)) {
        long top = view->vy + window_get_scroll_margin(e->window);
        if (view->cy > top) {
            move_up(view, view->cy - top);
        } else {
            block_iter_bof(&view->cursor);
        }
    }

    view_reset_preferred_x(view);
    return true;
}

static bool cmd_bookmark(EditorState *e, const CommandArgs *a)
{
    PointerArray *bookmarks = &e->bookmarks;
    if (has_flag(a, 'r')) {
        bookmark_pop(bookmarks, e->window);
        return true;
    }

    bookmark_push(bookmarks, get_current_file_location(e->view));
    return true;
}

static bool cmd_case(EditorState *e, const CommandArgs *a)
{
    change_case(e->view, last_flag_or_default(a, 't'));
    return true;
}

static void mark_tabbar_changed(Window *window, void* UNUSED_ARG(data))
{
    window->update_tabbar = true;
}

static bool cmd_cd(EditorState *e, const CommandArgs *a)
{
    const char *dir = a->args[0];
    if (unlikely(dir[0] == '\0')) {
        return error_msg(&e->err, "directory argument cannot be empty");
    }

    if (streq(dir, "-")) {
        dir = xgetenv("OLDPWD");
        if (!dir) {
            return error_msg(&e->err, "OLDPWD not set");
        }
    }

    char buf[8192];
    const char *cwd = getcwd(buf, sizeof(buf));
    if (chdir(dir) != 0) {
        return error_msg_errno(&e->err, "changing directory failed");
    }

    if (likely(cwd)) {
        int r = setenv("OLDPWD", cwd, 1);
        if (unlikely(r != 0)) {
            LOG_WARNING("failed to set OLDPWD: %s", strerror(errno));
        }
    }

    cwd = getcwd(buf, sizeof(buf));
    if (likely(cwd)) {
        int r = setenv("PWD", cwd, 1);
        if (unlikely(r != 0)) {
            LOG_WARNING("failed to set PWD: %s", strerror(errno));
        }
    }

    for (size_t i = 0, n = e->buffers.count; i < n; i++) {
        Buffer *buffer = e->buffers.ptrs[i];
        buffer_update_short_filename_cwd(buffer, &e->home_dir, cwd);
    }

    frame_for_each_window(e->root_frame, mark_tabbar_changed, NULL);
    e->screen_update |= UPDATE_TERM_TITLE;
    return true;
}

static bool cmd_center_view(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    e->view->force_center = true;
    return true;
}

static bool cmd_clear(EditorState *e, const CommandArgs *a)
{
    bool auto_indent = e->buffer->options.auto_indent && !has_flag(a, 'i');
    clear_lines(e->view, auto_indent);
    return true;
}

static bool cmd_close(EditorState *e, const CommandArgs *a)
{
    bool force = has_flag(a, 'f');
    if (!force && !view_can_close(e->view)) {
        bool prompt = has_flag(a, 'p');
        if (!prompt) {
            return error_msg (
                &e->err,
                "The buffer is modified; "
                "save or run 'close -f' to close without saving"
            );
        }

        if (unlikely(e->flags & EFLAG_HEADLESS)) {
            return error_msg(&e->err, "-p flag unavailable in headless mode");
        }

        static const char str[] = "Close without saving changes? [y/N]";
        if (dialog_prompt(e, str, "ny") != 'y') {
            return false;
        }
    }

    bool allow_quit = has_flag(a, 'q');
    if (allow_quit && e->buffers.count == 1 && e->root_frame->frames.count <= 1) {
        e->status = EDITOR_EXIT_OK;
        return true;
    }

    bool allow_wclose = has_flag(a, 'w');
    if (allow_wclose && e->window->views.count <= 1) {
        window_close(e->window);
        return true;
    }

    window_close_current_view(e->window);
    set_view(e->window->view);
    return true;
}

static bool cmd_command(EditorState *e, const CommandArgs *a)
{
    const char *text = a->args[0];
    push_input_mode(e, e->command_mode);
    if (text) {
        cmdline_set_text(&e->cmdline, text);
    }
    return true;
}

static bool cmd_compile(EditorState *e, const CommandArgs *a)
{
    Compiler *compiler = find_compiler(&e->compilers, a->args[0]);
    if (unlikely(!compiler)) {
        return error_msg(&e->err, "No such error parser %s", a->args[0]);
    }

    bool quiet = has_flag(a, 's');
    if ((e->flags & EFLAG_HEADLESS) && !quiet) {
        LOG_INFO("automatically added -s flag to compile command (headless mode)");
        quiet = true;
    }

    SpawnContext ctx = {
        .argv = (const char **)a->args + 1,
        .ebuf = &e->err,
        .quiet = quiet,
    };

    MessageArray *messages = &e->messages;
    clear_messages(messages);

    yield_terminal(e, quiet);
    bool prompt = has_flag(a, 'p');
    bool read_stdout = has_flag(a, '1');
    bool spawned = spawn_compiler(&ctx, compiler, messages, read_stdout);
    resume_terminal(e, quiet, spawned && prompt);

    activate_current_message_save(messages, &e->bookmarks, e->view);
    return spawned;
}

static bool cmd_copy(EditorState *e, const CommandArgs *a)
{
    const char *text = a->args[0];
    Terminal *term = &e->terminal;
    bool clipboard = has_flag(a, 'b');
    bool primary = has_flag(a, 'p');
    bool internal = has_flag(a, 'i') || !(clipboard || primary);
    bool osc52 = (clipboard || primary) && term->features & TFLAG_OSC52_COPY;

    if (text) {
        size_t len = strlen(text);
        if (internal) {
            record_copy(&e->clipboard, xstrdup(text), len, false);
        }
        if (osc52) {
            if (!term_osc52_copy(&term->obuf, text, len, clipboard, primary)) {
                error_msg_errno(&e->err, "OSC 52 copy failed");
            }
        }
        return true;
    }

    const View *view = e->view;
    BlockIter bi;
    size_t size;
    bool line_copy;
    if (view->selection) {
        SelectionInfo info = init_selection(view);
        size = info.eo - info.so;
        bi = info.si;
        line_copy = (view->selection == SELECT_LINES);
    } else {
        bi = view->cursor;
        block_iter_bol(&bi);
        BlockIter tmp = bi;
        size = block_iter_eat_line(&tmp);
        line_copy = true;
    }

    if (unlikely(size == 0)) {
        return true;
    }

    char *buf = block_iter_get_bytes(&bi, size);
    if (osc52) {
        if (!term_osc52_copy(&term->obuf, buf, size, clipboard, primary)) {
            error_msg_errno(&e->err, "OSC 52 copy failed");
        }
    }

    if (internal) {
        // Clipboard takes ownership of `buf`
        record_copy(&e->clipboard, buf, size, line_copy);
    } else {
        free(buf);
    }

    if (!has_flag(a, 'k')) {
        unselect(e->view);
    }

    // TODO: return false if term_osc52_copy() failed?
    return true;
}

static bool cmd_cursor(EditorState *e, const CommandArgs *a)
{
    if (unlikely(a->nr_args == 0)) {
        // Reset all cursor styles
        for (CursorInputMode m = 0; m < ARRAYLEN(e->cursor_styles); m++) {
            e->cursor_styles[m] = get_default_cursor_style(m);
        }
        e->screen_update |= UPDATE_CURSOR_STYLE;
        return true;
    }

    CursorInputMode mode = cursor_mode_from_str(a->args[0]);
    if (unlikely(mode >= NR_CURSOR_MODES)) {
        return error_msg(&e->err, "invalid mode argument: %s", a->args[0]);
    }

    TermCursorStyle style = get_default_cursor_style(mode);
    if (a->nr_args >= 2) {
        style.type = cursor_type_from_str(a->args[1]);
        if (unlikely(style.type == CURSOR_INVALID)) {
            return error_msg(&e->err, "invalid cursor type: %s", a->args[1]);
        }
    }

    if (a->nr_args >= 3) {
        style.color = cursor_color_from_str(a->args[2]);
        if (unlikely(style.color == COLOR_INVALID)) {
            return error_msg(&e->err, "invalid cursor color: %s", a->args[2]);
        }
    }

    e->cursor_styles[mode] = style;
    e->screen_update |= UPDATE_CURSOR_STYLE;
    return true;
}

static bool cmd_cut(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    View *view = e->view;
    long preferred_x = view_get_preferred_x(view);
    size_t size;
    bool line_copy;

    if (view->selection) {
        line_copy = (view->selection == SELECT_LINES);
        size = prepare_selection(view);
        unselect(view);
    } else {
        line_copy = true;
        block_iter_bol(&view->cursor);
        BlockIter tmp = view->cursor;
        size = block_iter_eat_line(&tmp);
    }

    if (size == 0) {
        return true;
    }

    char *buf = block_iter_get_bytes(&view->cursor, size);
    record_copy(&e->clipboard, buf, size, line_copy);
    buffer_delete_bytes(view, size);

    if (line_copy) {
        move_to_preferred_x(view, preferred_x);
    }

    return true;
}

static bool cmd_def_mode(EditorState *e, const CommandArgs *a)
{
    const char *name = a->args[0];
    if (name[0] == '\0' || name[0] == '-' ) {
        return error_msg(&e->err, "mode name can't be empty or start with '-'");
    }

    HashMap *modes = &e->modes;
    if (hashmap_get(modes, name)) {
        return error_msg(&e->err, "mode '%s' already exists", name);
    }

    PointerArray ftmodes = PTR_ARRAY_INIT;
    for (size_t i = 1, n = a->nr_args; i < n; i++) {
        const char *ftname = a->args[i];
        ModeHandler *mode = get_mode_handler(modes, ftname);
        if (unlikely(!mode)) {
            ptr_array_free_array(&ftmodes);
            return error_msg(&e->err, "unknown fallthrough mode '%s'", ftname);
        }
        if (unlikely(mode->cmds != &normal_commands)) {
            // TODO: Support "command" and "search" as fallback modes?
            // If implemented, all involved modes need to use the same
            // `CommandSet`.
            ptr_array_free_array(&ftmodes);
            return error_msg(&e->err, "unable to use '%s' as fall-through mode", ftname);
        }
        ptr_array_append(&ftmodes, mode);
    }

    static const FlagMapping map[] = {
        {'u', MHF_NO_TEXT_INSERTION},
        {'U', MHF_NO_TEXT_INSERTION | MHF_NO_TEXT_INSERTION_RECURSIVE},
    };

    ModeHandler *mode = new_mode(modes, xstrdup(name), &normal_commands);
    mode->flags = cmdargs_convert_flags(a, map, ARRAYLEN(map));
    mode->fallthrough_modes = ftmodes;
    return true;
}

static bool cmd_delete(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    delete_ch(e->view);
    return true;
}

static bool cmd_delete_eol(EditorState *e, const CommandArgs *a)
{
    View *view = e->view;
    if (view->selection) {
        // TODO: return false?
        return true;
    }

    bool delete_newline_if_at_eol = has_flag(a, 'n');
    BlockIter bi = view->cursor;
    if (delete_newline_if_at_eol) {
        CodePoint ch;
        if (block_iter_get_char(&view->cursor, &ch) == 1 && ch == '\n') {
            delete_ch(view);
            return true;
        }
    }

    buffer_delete_bytes(view, block_iter_eol(&bi));
    return true;
}

static bool cmd_delete_line(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    View *view = e->view;
    long x = view_get_preferred_x(view);
    bool whole_lines = true;
    size_t del_count;

    if (view->selection) {
        whole_lines = !has_flag(a, 'S');
        view->selection = whole_lines ? SELECT_LINES : view->selection;
        del_count = prepare_selection(view);
        unselect(view);
    } else {
        block_iter_bol(&view->cursor);
        BlockIter tmp = view->cursor;
        del_count = block_iter_eat_line(&tmp);
    }

    buffer_delete_bytes(view, del_count);
    if (whole_lines) {
        move_to_preferred_x(view, x);
    }

    return true;
}

static bool cmd_delete_word(EditorState *e, const CommandArgs *a)
{
    bool skip_non_word = has_flag(a, 's');
    BlockIter bi = e->view->cursor;
    buffer_delete_bytes(e->view, word_fwd(&bi, skip_non_word));
    return true;
}

static bool cmd_down(EditorState *e, const CommandArgs *a)
{
    handle_selection_flags(e->view, a);
    move_down(e->view, 1);
    return true;
}

static bool cmd_eof(EditorState *e, const CommandArgs *a)
{
    handle_selection_flags(e->view, a);
    move_eof(e->view);
    return true;
}

static bool cmd_eol(EditorState *e, const CommandArgs *a)
{
    handle_selection_flags(e->view, a);
    move_eol(e->view);
    return true;
}

static bool cmd_eolsf(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    View *view = e->view;
    handle_selection_flags(view, a);

    if (!block_iter_eol(&view->cursor)) {
        Window *window = e->window;
        long margin = window_get_scroll_margin(window);
        long bottom = view->vy + window->edit_h - 1 - margin;
        if (view->cy < bottom) {
            move_down(view, bottom - view->cy);
        } else {
            block_iter_eof(&view->cursor);
        }
    }

    view_reset_preferred_x(view);
    return true;
}

static bool cmd_erase(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    erase(e->view);
    return true;
}

static bool cmd_erase_bol(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    buffer_erase_bytes(e->view, block_iter_bol(&e->view->cursor));
    return true;
}

static bool cmd_erase_word(EditorState *e, const CommandArgs *a)
{
    View *view = e->view;
    bool skip_non_word = has_flag(a, 's');
    buffer_erase_bytes(view, word_bwd(&view->cursor, skip_non_word));
    return true;
}

static bool cmd_errorfmt(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args == 0 || a->nr_args > 2 + ERRORFMT_CAPTURE_MAX);
    const char *name = a->args[0];
    if (a->nr_args == 1) {
        remove_compiler(&e->compilers, name);
        return true;
    }

    static_assert(NR_ERRFMT_INDICES == 4);
    size_t max_idx = 0;
    int8_t indices[NR_ERRFMT_INDICES] = {
        [ERRFMT_FILE] = -1,
        [ERRFMT_LINE] = -1,
        [ERRFMT_COLUMN] = -1,
        [ERRFMT_MESSAGE] = 0,
    };

    for (size_t i = 0, n = a->nr_args - 2; i < n; i++) {
        char *cap_name = a->args[i + 2];
        if (streq(cap_name, "_")) {
            continue;
        }
        ssize_t cap_idx = errorfmt_capture_name_to_index(cap_name);
        if (unlikely(cap_idx < 0)) {
            return error_msg(&e->err, "unknown substring name %s", cap_name);
        }
        max_idx = i + 1;
        indices[cap_idx] = max_idx;
    }


    const char *pattern = a->args[1];
    regex_t re;
    if (unlikely(!regexp_compile(&e->err, &re, pattern, 0))) {
        return false;
    }

    if (unlikely(max_idx > re.re_nsub)) {
        regfree(&re);
        return error_msg(&e->err, "invalid substring count");
    }

    bool ignore = has_flag(a, 'i');
    add_error_fmt(&e->compilers, name, pattern, &re, indices, ignore);
    return true;
}

static bool cmd_exec(EditorState *e, const CommandArgs *a)
{
    ExecAction actions[3] = {EXEC_TTY, EXEC_TTY, EXEC_TTY};
    ExecFlags exec_flags = 0;
    bool lflag = false;
    bool move_after_insert = false;

    for (size_t i = 0, n = a->nr_flags, argidx = 0, fd; i < n; i++) {
        switch (a->flags[i]) {
            case 'e': fd = STDERR_FILENO; break;
            case 'i': fd = STDIN_FILENO; break;
            case 'o': fd = STDOUT_FILENO; break;
            case 'p': exec_flags |= EXECFLAG_PROMPT; continue;
            case 's': exec_flags |= EXECFLAG_QUIET; continue;
            case 't': exec_flags &= ~EXECFLAG_QUIET; continue;
            case 'l': lflag = true; continue;
            case 'm': move_after_insert = true; continue;
            case 'n': exec_flags |= EXECFLAG_STRIP_NL; continue;
            default: BUG("unexpected flag"); return false;
        }
        const char *action_name = a->args[argidx++];
        ExecAction action = lookup_exec_action(action_name, fd);
        if (unlikely(action == EXEC_INVALID)) {
            return error_msg(&e->err, "invalid action for -%c: '%s'", a->flags[i], action_name);
        }
        actions[fd] = action;
    }

    if (lflag && actions[STDIN_FILENO] == EXEC_BUFFER) {
        // For compat. with old "filter" and "pipe-to" commands
        actions[STDIN_FILENO] = EXEC_LINE;
    }

    if ((e->flags & EFLAG_HEADLESS) && !(exec_flags & EXECFLAG_QUIET)) {
        LOG_INFO("automatically added -s flag to exec command (headless mode)");
        exec_flags |= EXECFLAG_QUIET;
    }

    const char **argv = (const char **)a->args + a->nr_flag_args;
    ssize_t outlen = handle_exec(e, argv, actions, exec_flags);
    if (outlen <= 0) {
        return outlen == 0;
    }

    if (move_after_insert && actions[STDOUT_FILENO] == EXEC_BUFFER) {
        block_iter_skip_bytes(&e->view->cursor, outlen);
    }
    return true;
}

static bool cmd_ft(EditorState *e, const CommandArgs *a)
{
    ErrorBuffer *ebuf = &e->err;
    char **args = a->args;
    const char *filetype = args[0];
    if (unlikely(!is_valid_filetype_name(filetype))) {
        return error_msg(ebuf, "Invalid filetype name: '%s'", filetype);
    }

    FileDetectionType dt = FT_EXTENSION;
    switch (last_flag(a)) {
        case 'b': dt = FT_BASENAME; break;
        case 'c': dt = FT_CONTENT; break;
        case 'f': dt = FT_FILENAME; break;
        case 'i': dt = FT_INTERPRETER; break;
    }

    PointerArray *filetypes = &e->filetypes;
    size_t nfailed = 0;
    for (size_t i = 1, n = a->nr_args; i < n; i++) {
        if (!add_filetype(filetypes, filetype, args[i], dt, ebuf)) {
            nfailed++;
        }
    }

    return nfailed == 0;
}

static bool cmd_hi(EditorState *e, const CommandArgs *a)
{
    if (unlikely(a->nr_args == 0)) {
        exec_builtin_color_reset(e);
        goto update;
    }

    char **strs = a->args + 1;
    size_t strs_len = a->nr_args - 1;
    TermStyle style;
    ssize_t n = parse_term_style(&style, strs, strs_len);
    if (unlikely(n != strs_len)) {
        if (n < 0) {
            return error_msg(&e->err, "too many colors");
        }
        BUG_ON(n > strs_len);
        return error_msg(&e->err, "invalid color or attribute: '%s'", strs[n]);
    }

    TermFeatureFlags features = e->terminal.features;
    bool true_color = !!(features & TFLAG_TRUE_COLOR);
    bool optimize = (true_color && e->options.optimize_true_color);
    int32_t fg = color_to_nearest(style.fg, features, optimize);
    int32_t bg = color_to_nearest(style.bg, features, optimize);
    if (!true_color && has_flag(a, 'c') && (fg != style.fg || bg != style.bg)) {
        return true;
    }

    style.fg = fg;
    style.bg = bg;
    set_highlight_style(&e->styles, a->args[0], &style);

update:
    // Don't call update_all_syntax_styles() needlessly; it's called
    // right after config has been loaded
    if (e->status != EDITOR_INITIALIZING) {
        update_all_syntax_styles(&e->syntaxes, &e->styles);
        e->screen_update |= UPDATE_ALL_WINDOWS;
    }

    return true;
}

static bool cmd_include(EditorState *e, const CommandArgs *a)
{
    ConfigFlags flags = has_flag(a, 'q') ? CFG_NOFLAGS : CFG_MUST_EXIST;
    if (has_flag(a, 'b')) {
        flags |= CFG_BUILTIN;
    }

    int err = read_normal_config(e, a->args[0], flags);
    // TODO: Clean up read_normal_config() so this can be simplified to `err == 0`
    return err == 0 || (err == ENOENT && !(flags & CFG_MUST_EXIST));
}

static bool cmd_insert(EditorState *e, const CommandArgs *a)
{
    const char *str = a->args[0];
    if (has_flag(a, 'k')) {
        for (size_t i = 0; str[i]; i++) {
            insert_ch(e->view, str[i]);
        }
        return true;
    }

    bool move_after = has_flag(a, 'm');
    insert_text(e->view, str, strlen(str), move_after);
    return true;
}

static bool cmd_join(EditorState *e, const CommandArgs *a)
{
    const char *delim = a->args[0] ? a->args[0] : " ";
    join_lines(e->view, delim, strlen(delim));
    return true;
}

static bool cmd_left(EditorState *e, const CommandArgs *a)
{
    handle_selection_flags(e->view, a);
    move_cursor_left(e->view);
    return true;
}

static bool cmd_line(EditorState *e, const CommandArgs *a)
{
    const char *str = a->args[0];
    size_t line, column;
    if (unlikely(!str_to_xfilepos(str, &line, &column))) {
        return error_msg(&e->err, "Invalid line number: %s", str);
    }

    View *view = e->view;
    long x = view_get_preferred_x(view);
    handle_selection_flags(view, a);

    if (column >= 1) {
        // Column was specified; move to exact position
        move_to_filepos(view, line, column);
    } else {
        // Column was omitted; move to line while preserving current column
        move_to_line(view, line);
        move_to_preferred_x(view, x);
    }

    return true;
}

static bool cmd_macro(EditorState *e, const CommandArgs *a)
{
    MacroRecorder *m = &e->macro;
    const char *action = a->args[0];

    if (streq(action, "play") || streq(action, "run")) {
        for (size_t i = 0, n = m->macro.count; i < n; i++) {
            const char *cmd_str = m->macro.ptrs[i];
            if (!handle_normal_command(e, cmd_str, false)) {
                return false;
            }
        }
        return true;
    }

    if (streq(action, "toggle")) {
        action = m->recording ? "stop" : "record";
    }

    if (streq(action, "record")) {
        bool r = macro_record(m);
        return info_msg(&e->err, "%s", r ? "Recording macro" : "Already recording");
    }

    if (streq(action, "stop")) {
        if (!macro_stop(m)) {
            return info_msg(&e->err, "Not recording");
        }
        size_t count = m->macro.count;
        const char *plural = (count != 1) ? "s" : "";
        return info_msg(&e->err, "Macro recording stopped; %zu command%s saved", count, plural);
    }

    if (streq(action, "cancel")) {
        bool r = macro_cancel(m);
        return info_msg(&e->err, "%s", r ? "Macro recording cancelled" : "Not recording");
    }

    return error_msg(&e->err, "Unknown action '%s'", action);
}

static bool cmd_match_bracket(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    View *view = e->view;
    CodePoint cursor_char;
    if (!block_iter_get_char(&view->cursor, &cursor_char)) {
        return error_msg(&e->err, "No character under cursor");
    }

    CodePoint target = cursor_char;
    BlockIter bi = view->cursor;
    size_t level = 0;
    CodePoint u = 0;

    switch (cursor_char) {
    case '<':
    case '[':
    case '{':
        target++;
        // Fallthrough
    case '(':
        target++;
        goto search_fwd;
    case '>':
    case ']':
    case '}':
        target--;
        // Fallthrough
    case ')':
        target--;
        goto search_bwd;
    default:
        return error_msg(&e->err, "Character under cursor not matchable");
    }

search_fwd:
    block_iter_next_char(&bi, &u);
    BUG_ON(u != cursor_char);
    while (block_iter_next_char(&bi, &u)) {
        if (u == target) {
            if (level == 0) {
                block_iter_prev_char(&bi, &u);
                goto found;
            }
            level--;
        } else if (u == cursor_char) {
            level++;
        }
    }
    goto not_found;

search_bwd:
    while (block_iter_prev_char(&bi, &u)) {
        if (u == target) {
            if (level == 0) {
                goto found;
            }
            level--;
        } else if (u == cursor_char) {
            level++;
        }
    }

not_found:
    return error_msg(&e->err, "No matching bracket found");

found:
    handle_selection_flags(view, a);
    view->cursor = bi;
    return true;
}

static bool cmd_mode(EditorState *e, const CommandArgs *a)
{
    const char *name = a->args[0];
    ModeHandler *handler = get_mode_handler(&e->modes, name);
    if (!handler) {
        return error_msg(&e->err, "unknown mode '%s'", name);
    }

    push_input_mode(e, handler);
    return true;
}

static bool cmd_move_tab(EditorState *e, const CommandArgs *a)
{
    Window *window = e->window;
    const size_t ntabs = window->views.count;
    const char *str = a->args[0];
    size_t to, from = ptr_array_xindex(&window->views, e->view);
    if (streq(str, "left")) {
        to = size_decrement_wrapped(from, ntabs);
    } else if (streq(str, "right")) {
        to = size_increment_wrapped(from, ntabs);
    } else {
        if (!str_to_size(str, &to) || to == 0) {
            return error_msg(&e->err, "Invalid tab position %s", str);
        }
        to = MIN(to, ntabs) - 1;
    }
    ptr_array_move(&window->views, from, to);
    window->update_tabbar = true;
    return true;
}

static bool cmd_msg(EditorState *e, const CommandArgs *a)
{
    const char *str = a->args[0];
    uint_least64_t np = cmdargs_flagset_value('n') | cmdargs_flagset_value('p');
    if (u64_popcount(a->flag_set & np) + !!str >= 2) {
        return error_msg(&e->err, "flags [-n|-p] and [number] argument are mutually exclusive");
    }

    MessageArray *msgs = &e->messages;
    size_t count = msgs->array.count;
    if (count == 0) {
        return true;
    }

    size_t p = msgs->pos;
    BUG_ON(p >= count);
    if (has_flag(a, 'n')) {
        p = MIN(p + 1, count - 1);
    } else if (has_flag(a, 'p')) {
        p -= (p > 0);
    } else if (str) {
        if (!str_to_size(str, &p) || p == 0) {
            return error_msg(&e->err, "invalid message index: %s", str);
        }
        p = MIN(p - 1, count - 1);
    }

    msgs->pos = p;
    return activate_current_message(msgs, e->window);
}

static bool cmd_new_line(EditorState *e, const CommandArgs *a)
{
    new_line(e->view, has_flag(a, 'a'));
    return true;
}

static bool cmd_next(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    const PointerArray *views = &e->window->views;
    size_t current = ptr_array_xindex(views, e->view);
    size_t next = size_increment_wrapped(current, views->count);
    set_view(views->ptrs[next]);
    return true;
}

static bool xglob(ErrorBuffer *ebuf, char **args, glob_t *globbuf)
{
    BUG_ON(!args);
    BUG_ON(!args[0]);
    int err = glob(*args, GLOB_NOCHECK, NULL, globbuf);
    while (err == 0 && *++args) {
        err = glob(*args, GLOB_NOCHECK | GLOB_APPEND, NULL, globbuf);
    }

    if (likely(err == 0)) {
        BUG_ON(globbuf->gl_pathc == 0);
        BUG_ON(!globbuf->gl_pathv);
        BUG_ON(!globbuf->gl_pathv[0]);
        return true;
    }

    BUG_ON(err == GLOB_NOMATCH);
    globfree(globbuf);
    return error_msg(ebuf, "glob: %s", (err == GLOB_NOSPACE) ? strerror(ENOMEM) : "failed");
}

static bool cmd_open(EditorState *e, const CommandArgs *a)
{
    bool temporary = has_flag(a, 't');
    if (unlikely(temporary && a->nr_args > 0)) {
        return error_msg(&e->err, "'open -t' can't be used with filename arguments");
    }

    const char *requested_encoding = NULL;
    char **args = a->args;
    if (unlikely(a->nr_flag_args > 0)) {
        // The "-e" flag is the only one that takes an argument, so the
        // above condition implies it was used
        BUG_ON(!has_flag(a, 'e'));
        requested_encoding = args[a->nr_flag_args - 1];
        args += a->nr_flag_args;
    }

    const char *encoding = NULL; // (Auto-detect)
    if (requested_encoding) {
        EncodingType enctype = lookup_encoding(requested_encoding);
        if (enctype == UTF8) {
            encoding = encoding_from_type(UTF8);
        } else if (conversion_supported_by_iconv(requested_encoding, "UTF-8")) {
            encoding = encoding_normalize(requested_encoding);
        } else {
            if (errno == EINVAL) {
                return error_msg(&e->err, "Unsupported encoding '%s'", requested_encoding);
            }
            return error_msg (
                &e->err,
                "iconv conversion from '%s' failed: %s",
                requested_encoding,
                strerror(errno)
            );
        }
    }

    Window *window = e->window;
    if (a->nr_args == 0) {
        View *view = window_open_new_file(window);
        view->buffer->temporary = temporary;
        if (requested_encoding) {
            buffer_set_encoding(view->buffer, encoding, e->options.utf8_bom);
        }
        return true;
    }

    char **paths = args;
    glob_t globbuf;
    bool use_glob = has_flag(a, 'g');
    if (use_glob) {
        if (!xglob(&e->err, args, &globbuf)) {
            return false;
        }
        paths = globbuf.gl_pathv;
    }

    View *first_opened;
    if (!paths[1]) {
        // Previous view is remembered when opening single file
        first_opened = window_open_file(window, paths[0], encoding);
    } else {
        // It makes no sense to remember previous view when opening multiple files
        first_opened = window_open_files(window, paths, encoding);
    }

    if (use_glob) {
        globfree(&globbuf);
    }

    return !!first_opened;
}

static bool cmd_option(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args < 3);
    const char *arg0 = a->args[0];
    char **strs = a->args + 1;
    size_t nstrs = a->nr_args - 1;
    if (unlikely(arg0[0] == '\0')) {
        return error_msg(&e->err, "first argument cannot be empty");
    }
    if (unlikely(nstrs & 1)) {
        return error_msg(&e->err, "missing option value");
    }
    if (unlikely(!validate_local_options(&e->err, strs))) {
        return false;
    }

    PointerArray *opts = &e->file_options;
    if (has_flag(a, 'r')) {
        FileTypeOrFileName u = {.filename = regexp_intern(&e->err, arg0)};
        if (unlikely(!u.filename)) {
            return false;
        }
        add_file_options(opts, FOPTS_FILENAME, u, strs, nstrs);
        return true;
    }

    size_t errors = 0;
    for (size_t pos = 0, len = strlen(arg0); pos < len; ) {
        const StringView ft = get_delim(arg0, &pos, len, ',');
        if (unlikely(!is_valid_filetype_name_sv(&ft))) {
            error_msg(&e->err, "invalid filetype name: '%.*s'", (int)ft.length, ft.data);
            errors++;
            continue;
        }
        FileTypeOrFileName u = {.filetype = mem_intern(ft.data, ft.length)};
        add_file_options(opts, FOPTS_FILETYPE, u, strs, nstrs);
    }

    return !errors;
}

static bool cmd_blkdown(EditorState *e, const CommandArgs *a)
{
    View *view = e->view;
    handle_selection_flags(view, a);

    // If current line is blank, skip past consecutive blank lines
    StringView line;
    fetch_this_line(&view->cursor, &line);
    if (strview_isblank(&line)) {
        while (block_iter_next_line(&view->cursor)) {
            line = block_iter_get_line(&view->cursor);
            if (!strview_isblank(&line)) {
                break;
            }
        }
    }

    // Skip past non-blank lines
    while (block_iter_next_line(&view->cursor)) {
        line = block_iter_get_line(&view->cursor);
        if (strview_isblank(&line)) {
            break;
        }
    }

    // If we reach the last populated line in the buffer, move down one line
    BlockIter tmp = view->cursor;
    block_iter_eol(&tmp);
    block_iter_skip_bytes(&tmp, 1);
    if (block_iter_is_eof(&tmp)) {
        view->cursor = tmp;
    }

    return true;
}

static bool cmd_blkup(EditorState *e, const CommandArgs *a)
{
    View *view = e->view;
    handle_selection_flags(view, a);

    // If cursor is on the first line, just move to bol
    if (view->cy == 0) {
        block_iter_bol(&view->cursor);
        return true;
    }

    // If current line is blank, skip past consecutive blank lines
    StringView line;
    fetch_this_line(&view->cursor, &line);
    if (strview_isblank(&line)) {
        while (block_iter_prev_line(&view->cursor)) {
            line = block_iter_get_line(&view->cursor);
            if (!strview_isblank(&line)) {
                break;
            }
        }
    }

    // Skip past non-blank lines
    while (block_iter_prev_line(&view->cursor)) {
        line = block_iter_get_line(&view->cursor);
        if (strview_isblank(&line)) {
            break;
        }
    }

    return true;
}

static bool cmd_paste(EditorState *e, const CommandArgs *a)
{
    char ac_flag = cmdargs_pick_winning_flag(a, 'a', 'c');
    PasteLinesType type = PASTE_LINES_BELOW_CURSOR;
    if (ac_flag == 'a') {
        type = PASTE_LINES_ABOVE_CURSOR;
    } else if (ac_flag == 'c') {
        type = PASTE_LINES_INLINE;
    }

    bool move_after = has_flag(a, 'm');
    paste(&e->clipboard, e->view, type, move_after);
    return true;
}

static bool cmd_pgdown(EditorState *e, const CommandArgs *a)
{
    View *view = e->view;
    handle_selection_flags(view, a);

    Window *window = e->window;
    long margin = window_get_scroll_margin(window);
    long bottom = view->vy + window->edit_h - 1 - margin;
    long count;

    if (view->cy < bottom) {
        count = bottom - view->cy;
    } else {
        count = window->edit_h - 1 - margin * 2;
    }

    move_down(view, count);
    return true;
}

static bool cmd_pgup(EditorState *e, const CommandArgs *a)
{
    View *view = e->view;
    handle_selection_flags(view, a);

    Window *window = e->window;
    long margin = window_get_scroll_margin(window);
    long top = view->vy + margin;
    long count;

    if (view->cy > top) {
        count = view->cy - top;
    } else {
        count = window->edit_h - 1 - margin * 2;
    }

    move_up(view, count);
    return true;
}

static bool cmd_prev(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    const PointerArray *views = &e->window->views;
    size_t current = ptr_array_xindex(views, e->view);
    size_t prev = size_decrement_wrapped(current, views->count);
    set_view(views->ptrs[prev]);
    return true;
}

static View *window_find_modified_view(Window *window)
{
    if (buffer_modified(window->view->buffer)) {
        return window->view;
    }
    for (size_t i = 0, n = window->views.count; i < n; i++) {
        View *view = window->views.ptrs[i];
        if (buffer_modified(view->buffer)) {
            return view;
        }
    }
    return NULL;
}

static size_t count_modified_buffers(const PointerArray *buffers, View **first)
{
    View *modified = NULL;
    size_t nr_modified = 0;
    for (size_t i = 0, n = buffers->count; i < n; i++) {
        Buffer *buffer = buffers->ptrs[i];
        if (!buffer_modified(buffer)) {
            continue;
        }
        nr_modified++;
        if (!modified) {
            modified = buffer_get_first_view(buffer);
        }
    }

    BUG_ON(nr_modified > 0 && !modified);
    *first = modified;
    return nr_modified;
}

static bool cmd_quit(EditorState *e, const CommandArgs *a)
{
    static const FlagMapping fmap[] = {
        {'C', EFLAG_CMD_HIST},
        {'S', EFLAG_SEARCH_HIST},
        {'F', EFLAG_FILE_HIST},
        {'H', EFLAG_ALL_HIST},
    };

    int exit_code = EDITOR_EXIT_OK;
    if (a->nr_args) {
        if (!str_to_int(a->args[0], &exit_code)) {
            return error_msg(&e->err, "Not a valid integer argument: '%s'", a->args[0]);
        }
        int max = EDITOR_EXIT_MAX;
        if (exit_code < 0 || exit_code > max) {
            return error_msg(&e->err, "Exit code should be between 0 and %d", max);
        }
    }

    View *first_modified = NULL;
    size_t n = count_modified_buffers(&e->buffers, &first_modified);
    if (n == 0) {
        goto exit;
    }

    BUG_ON(!first_modified);
    const char *plural = (n > 1) ? "s" : "";
    if (has_flag(a, 'f')) {
        LOG_INFO("force quitting with %zu modified buffer%s", n, plural);
        goto exit;
    }

    // Activate a modified view (giving preference to the current view or
    // a view in the current window)
    View *view = window_find_modified_view(e->window);
    set_view(view ? view : first_modified);

    if (!has_flag(a, 'p')) {
        return error_msg(&e->err, "Save modified files or run 'quit -f' to quit without saving");
    }

    if (unlikely(e->flags & EFLAG_HEADLESS)) {
        return error_msg(&e->err, "-p flag unavailable in headless mode");
    }

    char question[128];
    xsnprintf (
        question, sizeof question,
        "Quit without saving %zu modified buffer%s? [y/N]",
        n, plural
    );

    if (dialog_prompt(e, question, "ny") != 'y') {
        return false;
    }
    LOG_INFO("quit prompt accepted with %zu modified buffer%s", n, plural);

exit:;
    EditorFlags histflags = cmdargs_convert_flags(a, fmap, ARRAYLEN(fmap));
    e->flags &= ~histflags;
    e->status = exit_code;
    return true;
}

static bool cmd_redo(EditorState *e, const CommandArgs *a)
{
    char *arg = a->args[0];
    unsigned long change_id = 0;
    if (arg) {
        if (!str_to_ulong(arg, &change_id) || change_id == 0) {
            return error_msg(&e->err, "Invalid change id: %s", arg);
        }
    }
    if (!redo(e->view, change_id)) {
        return false;
    }

    unselect(e->view);
    return true;
}

static bool cmd_refresh(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    e->screen_update |= UPDATE_ALL;
    return true;
}

static bool repeat_insert(EditorState *e, const char *str, unsigned int count, bool move_after)
{
    BUG_ON(count < 2);
    size_t str_len = strlen(str);
    size_t bufsize;
    if (unlikely(size_multiply_overflows(count, str_len, &bufsize))) {
        return error_msg(&e->err, "Repeated insert would overflow");
    }
    if (unlikely(bufsize == 0)) {
        return true;
    }

    char *buf = malloc(bufsize);
    if (unlikely(!buf)) {
        return error_msg_errno(&e->err, "malloc");
    }

    char tmp[4096];
    if (str_len == 1) {
        memset(buf, str[0], bufsize);
        LOG_DEBUG("Optimized %u inserts of 1 byte into 1 memset()", count);
        goto insert;
    } else if (bufsize < 2 * sizeof(tmp) || str_len > sizeof(tmp) / 8) {
        for (size_t i = 0; i < count; i++) {
            memcpy(buf + (i * str_len), str, str_len);
        }
        goto insert;
    }

    size_t strs_per_tmp = sizeof(tmp) / str_len;
    size_t tmp_len = strs_per_tmp * str_len;
    size_t tmps_per_buf = bufsize / tmp_len;
    size_t remainder = bufsize % tmp_len;

    // Create a block of text containing `strs_per_tmp` concatenated strs
    for (size_t i = 0; i < strs_per_tmp; i++) {
        memcpy(tmp + (i * str_len), str, str_len);
    }

    // Copy `tmps_per_buf` copies of `tmp` into `buf`
    for (size_t i = 0; i < tmps_per_buf; i++) {
        memcpy(buf + (i * tmp_len), tmp, tmp_len);
    }

    // Copy the remainder into `buf` (if any)
    if (remainder) {
        memcpy(buf + (tmps_per_buf * tmp_len), tmp, remainder);
    }

    LOG_DEBUG (
        "Optimized %u inserts of %zu bytes into %zu inserts of %zu bytes",
        count, str_len,
        tmps_per_buf, tmp_len
    );

insert:
    insert_text(e->view, buf, bufsize, move_after);
    free(buf);
    return true;
}

static bool cmd_reopen(EditorState *e, const CommandArgs* UNUSED_ARG(a))
{
    for (const FileHistoryEntry *h = e->file_history.last; h; h = h->prev) {
        const char *path = h->filename;
        // The combination of walking the history and doing a linear search
        // (with find_buffer()) here makes this O(m*n) in the worst case,
        // although n will typically be small and m is bounded by
        // FILEHIST_MAX_ENTRIES
        if (
            !find_buffer(&e->buffers, path) // Not already open in the editor
            && access(path, R_OK) == 0 // File exists and is readable
            && window_open_file(e->window, path, NULL) // Reopened successfully
        ) {
            return true;
        }
    }

    return error_msg(&e->err, "no reopenable files in history");
}

static bool cmd_repeat(EditorState *e, const CommandArgs *a)
{
    ErrorBuffer *ebuf = &e->err;
    unsigned int count;
    if (unlikely(!str_to_uint(a->args[0], &count))) {
        return error_msg(ebuf, "Not a valid repeat count: %s", a->args[0]);
    }
    if (unlikely(count == 0)) {
        return true;
    }

    const Command *cmd = find_normal_command(a->args[1]);
    if (unlikely(!cmd)) {
        return error_msg(ebuf, "No such command: %s", a->args[1]);
    }

    CommandArgs a2 = cmdargs_new(a->args + 2);
    if (unlikely(!parse_args(cmd, &a2, ebuf))) {
        return false;
    }

    if (count > 1 && cmd->cmd == cmd_insert && !has_flag(&a2, 'k')) {
        // Use optimized implementation for repeated "insert"
        return repeat_insert(e, a2.args[0], count, has_flag(&a2, 'm'));
    }

    while (count--) {
        command_func_call(e, ebuf, cmd, &a2);
    }

    // TODO: return false if fn() fails?
    return true;
}

static bool cmd_replace(EditorState *e, const CommandArgs *a)
{
    static const FlagMapping map[] = {
        {'b', REPLACE_BASIC},
        {'c', REPLACE_CONFIRM},
        {'g', REPLACE_GLOBAL},
        {'i', REPLACE_IGNORE_CASE},
    };

    const char *pattern = a->args[0];
    ReplaceFlags flags = cmdargs_convert_flags(a, map, ARRAYLEN(map));

    if (unlikely((flags & REPLACE_CONFIRM) && (e->flags & EFLAG_HEADLESS))) {
        return error_msg(&e->err, "-c flag unavailable in headless mode");
    }

    char *alloc = NULL;
    if (has_flag(a, 'e')) {
        size_t len = strlen(pattern);
        if (strn_contains_char_type(pattern, len, ASCII_REGEX)) {
            pattern = alloc = regexp_escape(pattern, len);
        }
        flags &= ~REPLACE_BASIC;
    }

    const char *replacement = a->args[1] ? a->args[1] : "";
    bool r = reg_replace(e->view, pattern, replacement, flags);
    free(alloc);
    return r;
}

static bool cmd_right(EditorState *e, const CommandArgs *a)
{
    handle_selection_flags(e->view, a);
    move_cursor_right(e->view);
    return true;
}

static bool stat_changed(const FileInfo *file, const struct stat *st)
{
    // Don't compare st_mode because we allow chmod 755 etc.
    return !timespecs_equal(get_stat_mtime(st), &file->mtime)
        || st->st_dev != file->dev
        || st->st_ino != file->ino
        || st->st_size != file->size;
}

static bool save_unmodified_buffer(Buffer *buffer, const char *filename)
{
    SaveUnmodifiedType type = buffer->options.save_unmodified;
    if (type == SAVE_NONE) {
        LOG_INFO("buffer unchanged; leaving file untouched");
        return true;
    }

    BUG_ON(type != SAVE_TOUCH);
    struct timespec times[2];
    if (unlikely(clock_gettime(CLOCK_REALTIME, &times[0]) != 0)) {
        LOG_ERRNO("aborting partial save; clock_gettime() failed");
        return false;
    }

    times[1] = times[0];
    if (unlikely(utimensat(AT_FDCWD, filename, times, 0) != 0)) {
        LOG_ERRNO("aborting partial save; utimensat() failed");
        return false;
    }

    buffer->file.mtime = times[0];
    LOG_INFO("buffer unchanged; mtime/atime updated");
    return true;
}

static bool cmd_save(EditorState *e, const CommandArgs *a)
{
    Buffer *buffer = e->buffer;
    if (unlikely(buffer->stdout_buffer)) {
        const char *f = buffer_filename(buffer);
        return info_msg(&e->err, "%s can't be saved; it will be piped to stdout on exit", f);
    }

    char du_flag = cmdargs_pick_winning_flag(a, 'd', 'u');
    bool crlf = du_flag ? (du_flag == 'd') : buffer->crlf_newlines;

    const char *requested_encoding = NULL;
    char **args = a->args;
    if (unlikely(a->nr_flag_args > 0)) {
        BUG_ON(!has_flag(a, 'e'));
        requested_encoding = args[a->nr_flag_args - 1];
        args += a->nr_flag_args;
    }

    const char *encoding = buffer->encoding;
    bool bom = buffer->bom;
    if (requested_encoding) {
        EncodingType et = lookup_encoding(requested_encoding);
        if (et == UTF8) {
            if (!encoding_is_utf8(encoding)) {
                // Encoding changed
                encoding = encoding_from_type(UTF8);
                bom = e->options.utf8_bom;
            }
        } else if (conversion_supported_by_iconv("UTF-8", requested_encoding)) {
            encoding = encoding_normalize(requested_encoding);
            if (encoding != buffer->encoding) {
                // Encoding changed
                bom = !!get_bom_for_encoding(lookup_encoding(encoding));
            }
        } else {
            if (errno == EINVAL) {
                return error_msg(&e->err, "Unsupported encoding '%s'", requested_encoding);
            }
            return error_msg (
                &e->err,
                "iconv conversion to '%s' failed: %s",
                requested_encoding,
                strerror(errno)
            );
        }
    }

    char b_flag = cmdargs_pick_winning_flag(a, 'b', 'B');
    bom = b_flag ? (b_flag == 'b') : bom;

    char *absolute = buffer->abs_filename;
    bool force = has_flag(a, 'f');
    bool new_locked = false;
    if (a->nr_args > 0) {
        if (args[0][0] == '\0') {
            return error_msg(&e->err, "Empty filename not allowed");
        }
        char *tmp = path_absolute(args[0]);
        if (!tmp) {
            return error_msg_errno(&e->err, "Failed to make absolute path");
        }
        if (absolute && streq(tmp, absolute)) {
            free(tmp);
        } else {
            absolute = tmp;
        }
    } else {
        if (!absolute) {
            if (!has_flag(a, 'p')) {
                return error_msg(&e->err, "No filename");
            }
            push_input_mode(e, e->command_mode);
            cmdline_set_text(&e->cmdline, "save ");
            return true;
        }
        if (buffer->readonly && !force) {
            return error_msg(&e->err, "Use -f to force saving read-only file");
        }
    }

    mode_t old_mode = buffer->file.mode;
    bool hardlinks = false;
    struct stat st;
    bool stat_ok = !stat(absolute, &st);
    if (!stat_ok) {
        if (errno != ENOENT) {
            error_msg(&e->err, "stat failed for %s: %s", absolute, strerror(errno));
            goto error;
        }
    } else {
        if (
            absolute == buffer->abs_filename
            && !force
            && stat_changed(&buffer->file, &st)
        ) {
            error_msg (
                &e->err,
                "File has been modified by another process; "
                "use 'save -f' to force overwrite"
            );
            goto error;
        }
        if (S_ISDIR(st.st_mode)) {
            error_msg(&e->err, "Will not overwrite directory %s", absolute);
            goto error;
        }
        hardlinks = (st.st_nlink >= 2);
    }

    if (e->options.lock_files) {
        if (absolute == buffer->abs_filename) {
            if (!buffer->locked) {
                if (!lock_file(&e->locks_ctx, &e->err, absolute)) {
                    if (!force) {
                        error_msg(&e->err, "Can't lock file %s", absolute);
                        goto error;
                    }
                } else {
                    buffer->locked = true;
                }
            }
        } else {
            if (!lock_file(&e->locks_ctx, &e->err, absolute)) {
                if (!force) {
                    error_msg(&e->err, "Can't lock file %s", absolute);
                    goto error;
                }
            } else {
                new_locked = true;
            }
        }
    }

    if (stat_ok) {
        if (absolute != buffer->abs_filename && !force) {
            error_msg(&e->err, "Use -f to overwrite %s", absolute);
            goto error;
        }
        // Allow chmod 755 etc.
        buffer->file.mode = st.st_mode;
    }

    if (
        stat_ok
        && buffer->options.save_unmodified != SAVE_FULL
        && !stat_changed(&buffer->file, &st)
        && st.st_uid == buffer->file.uid
        && st.st_gid == buffer->file.gid
        && !buffer_modified(buffer)
        && absolute == buffer->abs_filename
        && encoding == buffer->encoding
        && crlf == buffer->crlf_newlines
        && bom == buffer->bom
        && save_unmodified_buffer(buffer, absolute)
    ) {
        BUG_ON(new_locked);
        return true;
    }

    FileSaveContext ctx = {
        .ebuf = &e->err,
        .encoding = encoding,
        .new_file_mode = e->new_file_mode,
        .crlf = crlf,
        .write_bom = bom,
        .hardlinks = hardlinks,
    };

    if (!save_buffer(buffer, absolute, &ctx)) {
        goto error;
    }

    buffer->saved_change = buffer->cur_change;
    buffer->readonly = false;
    buffer->temporary = false;
    buffer->crlf_newlines = crlf;
    buffer->bom = bom;
    if (requested_encoding) {
        buffer->encoding = encoding;
    }

    if (absolute != buffer->abs_filename) {
        if (buffer->locked) {
            // Filename changes, release old file lock
            unlock_file(&e->locks_ctx, &e->err, buffer->abs_filename);
        }
        buffer->locked = new_locked;

        free(buffer->abs_filename);
        buffer->abs_filename = absolute;
        buffer_update_short_filename(buffer, &e->home_dir);
        e->screen_update |= UPDATE_TERM_TITLE;

        // Filename change is not detected (only buffer_modified() change)
        buffer_mark_tabbars_changed(buffer);
    }
    if (!old_mode && streq(buffer->options.filetype, "none")) {
        // New file and most likely user has not changed the filetype
        if (buffer_detect_filetype(buffer, &e->filetypes)) {
            set_file_options(e, buffer);
            set_editorconfig_options(buffer);
            buffer_update_syntax(e, buffer);
        }
    }

    return true;

error:
    if (new_locked) {
        unlock_file(&e->locks_ctx, &e->err, absolute);
    }
    if (absolute != buffer->abs_filename) {
        free(absolute);
    }
    return false;
}

static bool cmd_scroll_down(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    View *view = e->view;
    handle_selection_flags(view, a);
    view->vy++;

    if (has_flag(a, 'M')) {
        return true; // Don't move cursor, even at scroll margin
    }

    unsigned int margin = window_get_scroll_margin(e->window);
    if (view->cy < view->vy + margin) {
        move_down(view, 1);
    }

    return true;
}

static bool cmd_scroll_pgdown(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    View *view = e->view;
    handle_selection_flags(view, a);

    const long edit_h = e->window->edit_h;
    const long max = view->buffer->nl - edit_h + 1;
    long count;

    if (view->vy < max && max > 0) {
        bool half = has_flag(a, 'h');
        unsigned int shift = half & 1;
        count = (edit_h - 1) >> shift;
        if (view->vy + count > max) {
            count = max - view->vy;
        }
        view->vy += count;
    } else if (view->cy < view->buffer->nl) {
        count = view->buffer->nl - view->cy;
    } else {
        return true;
    }

    move_down(view, count);
    return true;
}

static bool cmd_scroll_pgup(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    View *view = e->view;
    handle_selection_flags(view, a);

    long count;
    if (view->vy > 0) {
        bool half = has_flag(a, 'h');
        unsigned int shift = half & 1;
        count = MIN((e->window->edit_h - 1) >> shift, view->vy);
        view->vy -= count;
    } else if (view->cy > 0) {
        count = view->cy;
    } else {
        return true;
    }

    move_up(view, count);
    return true;
}

static bool cmd_scroll_up(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    View *view = e->view;
    handle_selection_flags(view, a);
    view->vy -= (view->vy > 0);

    if (has_flag(a, 'M')) {
        return true; // Don't move cursor, even at scroll margin
    }

    const Window *window = e->window;
    unsigned int margin = window_get_scroll_margin(window);
    if (view->vy + (window->edit_h - margin) <= view->cy) {
        move_up(view, 1);
    }

    return true;
}

static unsigned int count_npw_flags(const CommandArgs *a)
{
    uint_least64_t npw = 0;
    npw |= cmdargs_flagset_value('n');
    npw |= cmdargs_flagset_value('p');
    npw |= cmdargs_flagset_value('w');
    return u64_popcount(a->flag_set & npw);
}

static bool cmd_search(EditorState *e, const CommandArgs *a)
{
    const char *pattern = a->args[0];
    if (count_npw_flags(a) + !!pattern > 1) {
        return error_msg(&e->err, "flags [-n|-p|-w] and [pattern] argument are mutually exclusive");
    }

    View *view = e->view;
    char pattbuf[4096];
    bool use_word_under_cursor = has_flag(a, 'w');

    if (use_word_under_cursor) {
        StringView word = view_get_word_under_cursor(view);
        if (word.length == 0) {
            // Error message would not be very useful here
            return false;
        }
        const RegexpWordBoundaryTokens *rwbt = &e->regexp_word_tokens;
        const size_t bmax = sizeof(rwbt->start);
        static_assert_compatible_types(rwbt->start, char[8]);
        if (unlikely(word.length >= sizeof(pattbuf) - (bmax * 2))) {
            return error_msg(&e->err, "word under cursor too long");
        }
        char *ptr = stpncpy(pattbuf, rwbt->start, bmax);
        memcpy(ptr, word.data, word.length);
        memcpy(ptr + word.length, rwbt->end, bmax);
        pattern = pattbuf;
    }

    SearchState *search = &e->search;
    SearchCaseSensitivity cs = e->options.case_sensitive_search;
    unselect(view);

    if (has_flag(a, 'n')) {
        return search_next(view, search, cs);
    }
    if (has_flag(a, 'p')) {
        return search_prev(view, search, cs);
    }

    search->reverse = has_flag(a, 'r');
    if (!pattern) {
        push_input_mode(e, e->search_mode);
        return true;
    }

    char *alloc = NULL;
    if (!use_word_under_cursor && has_flag(a, 'e')) {
        size_t len = strlen(pattern);
        if (strn_contains_char_type(pattern, len, ASCII_REGEX)) {
            pattern = alloc = regexp_escape(pattern, len);
        }
    }

    if (!has_flag(a, 'H')) {
        history_append(&e->search_history, pattern);
    }

    search_set_regexp(search, pattern);
    free(alloc);
    return has_flag(a, 's') || do_search_next(view, search, cs, use_word_under_cursor);
}

static bool cmd_select_block(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    select_block(e->view);

    // TODO: return false if select_block() doesn't select anything?
    return true;
}

static bool cmd_select(EditorState *e, const CommandArgs *a)
{
    View *view = e->view;
    SelectionType sel = has_flag(a, 'l') ? SELECT_LINES : SELECT_CHARS;
    bool keep = has_flag(a, 'k');
    if (!keep && view->selection && view->selection == sel) {
        sel = SELECT_NONE;
    }

    view->select_mode = sel;
    do_selection(view, sel);
    return true;
}

static bool cmd_set(EditorState *e, const CommandArgs *a)
{
    bool global = has_flag(a, 'g');
    bool local = has_flag(a, 'l');
    if (!e->buffer) {
        if (unlikely(local)) {
            return error_msg(&e->err, "Flag -l makes no sense in config file");
        }
        global = true;
    }

    char **args = a->args;
    size_t count = a->nr_args;
    if (count == 1) {
        return set_bool_option(e, args[0], local, global);
    }
    if (count & 1) {
        return error_msg(&e->err, "One or even number of arguments expected");
    }

    size_t errors = 0;
    for (size_t i = 0; i < count; i += 2) {
        if (!set_option(e, args[i], args[i + 1], local, global)) {
            errors++;
        }
    }

    return !errors;
}

static bool cmd_setenv(EditorState *e, const CommandArgs *a)
{
    const char *name = a->args[0];
    if (unlikely(streq(name, "DTE_VERSION"))) {
        return error_msg(&e->err, "$DTE_VERSION cannot be changed");
    }

    const size_t nr_args = a->nr_args;
    int res;
    if (nr_args == 2) {
        res = setenv(name, a->args[1], 1);
    } else {
        BUG_ON(nr_args != 1);
        res = unsetenv(name);
    }

    if (likely(res == 0)) {
        return true;
    }

    if (errno == EINVAL) {
        return error_msg(&e->err, "Invalid environment variable name '%s'", name);
    }

    return error_msg_errno(&e->err, nr_args == 2 ? "setenv" : "unsetenv");
}

static bool cmd_shift(EditorState *e, const CommandArgs *a)
{
    const char *arg = a->args[0];
    int count;
    if (!str_to_int(arg, &count)) {
        return error_msg(&e->err, "Invalid number: %s", arg);
    }
    if (count == 0) {
        return error_msg(&e->err, "Count must be non-zero");
    }
    shift_lines(e->view, count);
    return true;
}

static bool cmd_show(EditorState *e, const CommandArgs *a)
{
    bool write_to_cmdline = has_flag(a, 'c');
    if (write_to_cmdline && a->nr_args < 2) {
        return error_msg(&e->err, "\"show -c\" requires 2 arguments");
    }
    return show(e, a->args[0], a->args[1], write_to_cmdline);
}

static bool cmd_suspend(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    if (e->flags & EFLAG_HEADLESS) {
        return error_msg(&e->err, "unavailable in headless mode");
    }

    if (e->status == EDITOR_INITIALIZING) {
        LOG_WARNING("suspend request ignored");
        return false;
    }

    if (e->session_leader) {
        return error_msg(&e->err, "Session leader can't suspend");
    }

    ui_end(e);
    term_cooked();
    LOG_INFO("suspending");

    bool suspended = !kill(0, SIGSTOP);
    if (suspended) {
        LOG_INFO("resumed");
    } else {
        error_msg_errno(&e->err, "kill");
    }

    term_raw();
    ui_start(e);
    return suspended;
}

static bool cmd_tag(EditorState *e, const CommandArgs *a)
{
    if (has_flag(a, 'r')) {
        bookmark_pop(&e->bookmarks, e->window);
        return true;
    }

    size_t nargs = a->nr_args;
    StringView word_under_cursor;
    if (nargs == 0) {
        word_under_cursor = view_get_word_under_cursor(e->view);
        if (word_under_cursor.length == 0) {
            return false;
        }
    }

    MessageArray *msgs = &e->messages;
    clear_messages(msgs);
    if (!load_tag_file(&e->tagfile, &e->err)) {
        return false;
    }

    const char *filename = e->buffer->abs_filename;
    if (nargs == 0) {
        tag_lookup(&e->tagfile, msgs, &e->err, &word_under_cursor, filename);
    }

    for (size_t i = 0; i < nargs; i++) {
        StringView tagname = strview_from_cstring(a->args[i]);
        tag_lookup(&e->tagfile, msgs, &e->err, &tagname, filename);
    }

    activate_current_message_save(msgs, &e->bookmarks, e->view);
    return (msgs->array.count > 0);
}

static bool cmd_title(EditorState *e, const CommandArgs *a)
{
    Buffer *buffer = e->buffer;
    if (buffer->abs_filename) {
        return error_msg(&e->err, "saved buffers can't be retitled");
    }

    buffer_set_display_filename(buffer, xstrdup(a->args[0]));
    buffer_mark_tabbars_changed(buffer);
    e->screen_update |= UPDATE_TERM_TITLE;
    return true;
}

static bool cmd_toggle(EditorState *e, const CommandArgs *a)
{
    bool global = has_flag(a, 'g');
    bool verbose = has_flag(a, 'v');
    const char *option_name = a->args[0];
    size_t nr_values = a->nr_args - 1;
    if (nr_values == 0) {
        return toggle_option(e, option_name, global, verbose);
    }

    char **values = a->args + 1;
    return toggle_option_values(e, option_name, global, verbose, values, nr_values);
}

static bool cmd_undo(EditorState *e, const CommandArgs *a)
{
    View *view = e->view;
    bool move_only = has_flag(a, 'm');
    if (move_only) {
        const Change *change = view->buffer->cur_change;
        if (!change->next) {
            // If there's only 1 change, there's nothing meaningful to move to
            return false;
        }
        block_iter_goto_offset(&view->cursor, change->offset);
        view_reset_preferred_x(view);
        return true;
    }

    if (!undo(view)) {
        return false;
    }

    unselect(view);
    return true;
}

static bool cmd_unselect(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    unselect(e->view);
    return true;
}

static bool cmd_up(EditorState *e, const CommandArgs *a)
{
    handle_selection_flags(e->view, a);
    move_up(e->view, 1);
    return true;
}

static bool cmd_view(EditorState *e, const CommandArgs *a)
{
    Window *window = e->window;
    BUG_ON(window->views.count == 0);
    const char *arg = a->args[0];
    size_t idx;
    if (streq(arg, "last")) {
        idx = window->views.count - 1;
    } else {
        if (!str_to_size(arg, &idx) || idx == 0) {
            return error_msg(&e->err, "Invalid view index: %s", arg);
        }
        idx = MIN(idx, window->views.count) - 1;
    }
    set_view(window->views.ptrs[idx]);
    return true;
}

static bool cmd_wclose(EditorState *e, const CommandArgs *a)
{
    View *view = window_find_unclosable_view(e->window);
    bool force = has_flag(a, 'f');
    if (!view || force) {
        goto close;
    }

    bool prompt = has_flag(a, 'p');
    set_view(view);
    if (!prompt) {
        return error_msg (
            &e->err,
            "Save modified files or run 'wclose -f' to close "
            "window without saving"
        );
    }

    if (unlikely(e->flags & EFLAG_HEADLESS)) {
        return error_msg(&e->err, "-p flag unavailable in headless mode");
    }

    if (dialog_prompt(e, "Close window without saving? [y/N]", "ny") != 'y') {
        return false;
    }

close:
    window_close(e->window);
    return true;
}

static bool cmd_wflip(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    Frame *frame = e->window->frame;
    if (!frame->parent) {
        return false;
    }
    frame->parent->vertical ^= 1;
    e->screen_update |= UPDATE_ALL_WINDOWS;
    return true;
}

static bool cmd_wnext(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    e->window = window_next(e->window);
    set_view(e->window->view);
    e->screen_update |= UPDATE_ALL;
    frame_debug(e->root_frame);
    return true;
}

static bool cmd_word_bwd(EditorState *e, const CommandArgs *a)
{
    handle_selection_flags(e->view, a);
    bool skip_non_word = has_flag(a, 's');
    word_bwd(&e->view->cursor, skip_non_word);
    view_reset_preferred_x(e->view);
    return true;
}

static bool cmd_word_fwd(EditorState *e, const CommandArgs *a)
{
    handle_selection_flags(e->view, a);
    bool skip_non_word = has_flag(a, 's');
    word_fwd(&e->view->cursor, skip_non_word);
    view_reset_preferred_x(e->view);
    return true;
}

static bool cmd_wprev(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    e->window = window_prev(e->window);
    set_view(e->window->view);
    e->screen_update |= UPDATE_ALL;
    frame_debug(e->root_frame);
    return true;
}

static bool cmd_wrap_paragraph(EditorState *e, const CommandArgs *a)
{
    const char *arg = a->args[0];
    unsigned int width = e->buffer->options.text_width;
    if (arg) {
        if (!str_to_uint(arg, &width)) {
            return error_msg(&e->err, "invalid paragraph width: %s", arg);
        }
        unsigned int max = TEXT_WIDTH_MAX;
        if (width < 1 || width > max) {
            return error_msg(&e->err, "width must be between 1 and %u", max);
        }
    }
    wrap_paragraph(e->view, width);
    return true;
}

static bool cmd_wresize(EditorState *e, const CommandArgs *a)
{
    Window *window = e->window;
    if (!window->frame->parent) {
        // Only window
        return false;
    }

    ResizeDirection dir = RESIZE_DIRECTION_AUTO;
    switch (last_flag(a)) {
    case 'h':
        dir = RESIZE_DIRECTION_HORIZONTAL;
        break;
    case 'v':
        dir = RESIZE_DIRECTION_VERTICAL;
        break;
    }

    const char *arg = a->args[0];
    if (arg) {
        int n;
        if (!str_to_int(arg, &n)) {
            return error_msg(&e->err, "Invalid resize value: %s", arg);
        }
        if (arg[0] == '+' || arg[0] == '-') {
            frame_add_to_size(window->frame, dir, n);
        } else {
            frame_resize(window->frame, dir, n);
        }
    } else {
        frame_equalize_sizes(window->frame->parent);
    }

    e->screen_update |= UPDATE_ALL_WINDOWS;
    frame_debug(e->root_frame);
    // TODO: return false if resize failed?
    return true;
}

static bool cmd_wsplit(EditorState *e, const CommandArgs *a)
{
    bool before = has_flag(a, 'b');
    bool use_glob = has_flag(a, 'g') && a->nr_args > 0;
    bool vertical = has_flag(a, 'h');
    bool root = has_flag(a, 'r');
    bool temporary = has_flag(a, 't');
    bool empty = temporary || has_flag(a, 'n');

    if (unlikely(empty && a->nr_args > 0)) {
        return error_msg(&e->err, "flags -n and -t can't be used with filename arguments");
    }

    char **paths = a->args;
    glob_t globbuf;
    if (use_glob) {
        if (!xglob(&e->err, a->args, &globbuf)) {
            return false;
        }
        paths = globbuf.gl_pathv;
    }

    Frame *frame;
    if (root) {
        frame = frame_split_root(e, vertical, before);
    } else {
        frame = frame_split(e->window, vertical, before);
    }

    View *save = e->view;
    e->window = frame->window;
    e->view = NULL;
    e->buffer = NULL;
    e->screen_update |= UPDATE_ALL;

    View *view;
    if (empty) {
        view = window_open_new_file(e->window);
        view->buffer->temporary = temporary;
    } else if (paths[0]) {
        view = window_open_files(e->window, paths, NULL);
    } else {
        view = window_add_buffer(e->window, save->buffer);
        view->cursor = save->cursor;
        set_view(view);
    }

    if (use_glob) {
        globfree(&globbuf);
    }

    if (!view) {
        // Open failed, remove new window
        frame_remove(e, e->window->frame);
        e->view = save;
        e->buffer = save->buffer;
        e->window = save->window;
    }

    frame_debug(e->root_frame);
    return !!view;
}

static bool cmd_wswap(EditorState *e, const CommandArgs *a)
{
    BUG_ON(a->nr_args);
    Frame *frame = e->window->frame;
    Frame *parent = frame->parent;
    if (!parent) {
        return false;
    }

    PointerArray *pframes = &parent->frames;
    size_t current = ptr_array_xindex(pframes, frame);
    size_t next = size_increment_wrapped(current, pframes->count);
    ptr_array_swap(pframes, current, next);
    e->screen_update |= UPDATE_ALL_WINDOWS;
    return true;
}

enum {
    // Short aliases for CommandOptions:
    NA = 0,
    RC = CMDOPT_ALLOW_IN_RC,
    NFAA = CMDOPT_NO_FLAGS_AFTER_ARGS,
};

static const Command cmds[] = {
    {"alias", "", RC | NFAA, 1, 2, cmd_alias},
    {"bind", "cnqsT=", RC | NFAA, 1, 2, cmd_bind},
    {"blkdown", "cl", NA, 0, 0, cmd_blkdown},
    {"blkup", "cl", NA, 0, 0, cmd_blkup},
    {"bof", "cl", NA, 0, 0, cmd_bof},
    {"bol", "clrst", NA, 0, 0, cmd_bol},
    {"bolsf", "cl", NA, 0, 0, cmd_bolsf},
    {"bookmark", "r", NA, 0, 0, cmd_bookmark},
    {"case", "lu", NA, 0, 0, cmd_case},
    {"cd", "", RC, 1, 1, cmd_cd},
    {"center-view", "", NA, 0, 0, cmd_center_view},
    {"clear", "i", NA, 0, 0, cmd_clear},
    {"close", "fpqw", NA, 0, 0, cmd_close},
    {"command", "", NFAA, 0, 1, cmd_command},
    {"compile", "1ps", NFAA, 2, -1, cmd_compile},
    {"copy", "bikp", NA, 0, 1, cmd_copy},
    {"cursor", "", RC, 0, 3, cmd_cursor},
    {"cut", "", NA, 0, 0, cmd_cut},
    {"def-mode", "Uu", RC, 1, 16, cmd_def_mode},
    {"delete", "", NA, 0, 0, cmd_delete},
    {"delete-eol", "n", NA, 0, 0, cmd_delete_eol},
    {"delete-line", "S", NA, 0, 0, cmd_delete_line},
    {"delete-word", "s", NA, 0, 0, cmd_delete_word},
    {"down", "cl", NA, 0, 0, cmd_down},
    {"eof", "cl", NA, 0, 0, cmd_eof},
    {"eol", "cl", NA, 0, 0, cmd_eol},
    {"eolsf", "cl", NA, 0, 0, cmd_eolsf},
    {"erase", "", NA, 0, 0, cmd_erase},
    {"erase-bol", "", NA, 0, 0, cmd_erase_bol},
    {"erase-word", "s", NA, 0, 0, cmd_erase_word},
    {"errorfmt", "i", RC, 1, 2 + ERRORFMT_CAPTURE_MAX, cmd_errorfmt},
    {"exec", "e=i=o=lmnpst", NFAA, 1, -1, cmd_exec},
    {"ft", "bcfi", RC | NFAA, 2, -1, cmd_ft},
    {"hi", "c", RC | NFAA, 0, -1, cmd_hi},
    {"include", "bq", RC, 1, 1, cmd_include},
    {"insert", "km", NA, 1, 1, cmd_insert},
    {"join", "", NA, 0, 1, cmd_join},
    {"left", "cl", NA, 0, 0, cmd_left},
    {"line", "cl", NA, 1, 1, cmd_line},
    {"macro", "", NA, 1, 1, cmd_macro},
    {"match-bracket", "cl", NA, 0, 0, cmd_match_bracket},
    {"mode", "", RC, 1, 1, cmd_mode},
    {"move-tab", "", NA, 1, 1, cmd_move_tab},
    {"msg", "np", NA, 0, 1, cmd_msg},
    {"new-line", "a", NA, 0, 0, cmd_new_line},
    {"next", "", NA, 0, 0, cmd_next},
    {"open", "e=gt", NA, 0, -1, cmd_open},
    {"option", "r", RC | NFAA, 3, -1, cmd_option},
    {"paste", "acm", NA, 0, 0, cmd_paste},
    {"pgdown", "cl", NA, 0, 0, cmd_pgdown},
    {"pgup", "cl", NA, 0, 0, cmd_pgup},
    {"prev", "", NA, 0, 0, cmd_prev},
    {"quit", "CFHSfp", NA, 0, 1, cmd_quit},
    {"redo", "", NA, 0, 1, cmd_redo},
    {"refresh", "", NA, 0, 0, cmd_refresh},
    {"reopen", "", NA, 0, 0, cmd_reopen},
    {"repeat", "", NFAA, 2, -1, cmd_repeat},
    {"replace", "bcegi", NA, 1, 2, cmd_replace},
    {"right", "cl", NA, 0, 0, cmd_right},
    {"save", "Bbde=fpu", NA, 0, 1, cmd_save},
    {"scroll-down", "Mcl", NA, 0, 0, cmd_scroll_down},
    {"scroll-pgdown", "chl", NA, 0, 0, cmd_scroll_pgdown},
    {"scroll-pgup", "chl", NA, 0, 0, cmd_scroll_pgup},
    {"scroll-up", "Mcl", NA, 0, 0, cmd_scroll_up},
    {"search", "Henprsw", NA, 0, 1, cmd_search},
    {"select", "kl", NA, 0, 0, cmd_select},
    {"select-block", "", NA, 0, 0, cmd_select_block},
    {"set", "gl", RC, 1, -1, cmd_set},
    {"setenv", "", RC, 1, 2, cmd_setenv},
    {"shift", "", NA, 1, 1, cmd_shift},
    {"show", "c", NA, 1, 2, cmd_show},
    {"suspend", "", NA, 0, 0, cmd_suspend},
    {"tag", "r", NA, 0, -1, cmd_tag},
    {"title", "", NA, 1, 1, cmd_title},
    {"toggle", "gv", NA, 1, -1, cmd_toggle},
    {"undo", "m", NA, 0, 0, cmd_undo},
    {"unselect", "", NA, 0, 0, cmd_unselect},
    {"up", "cl", NA, 0, 0, cmd_up},
    {"view", "", NA, 1, 1, cmd_view},
    {"wclose", "fp", NA, 0, 0, cmd_wclose},
    {"wflip", "", NA, 0, 0, cmd_wflip},
    {"wnext", "", NA, 0, 0, cmd_wnext},
    {"word-bwd", "cls", NA, 0, 0, cmd_word_bwd},
    {"word-fwd", "cls", NA, 0, 0, cmd_word_fwd},
    {"wprev", "", NA, 0, 0, cmd_wprev},
    {"wrap-paragraph", "", NA, 0, 1, cmd_wrap_paragraph},
    {"wresize", "hv", NA, 0, 1, cmd_wresize},
    {"wsplit", "bghnrt", NA, 0, -1, cmd_wsplit},
    {"wswap", "", NA, 0, 0, cmd_wswap},
};

static bool allow_macro_recording(const Command *cmd, char **args)
{
    const CommandFunc fn = cmd->cmd;
    if (fn == cmd_macro || fn == cmd_command || fn == cmd_mode) {
        return false;
    }

    if (fn == cmd_search) {
        char **args_copy = copy_string_array(args, string_array_length(args));
        CommandArgs a = cmdargs_new(args_copy);
        bool ret = true;
        if (do_parse_args(cmd, &a) == ARGERR_NONE) {
            if (a.nr_args + count_npw_flags(&a) == 0) {
                // If command is "search" with no pattern argument and without
                // flags -n, -p or -w, the command would put the editor into
                // search mode, which shouldn't be recorded.
                ret = false;
            }
        }
        free_string_array(args_copy);
        return ret;
    }

    if (fn == cmd_exec) {
        // TODO: don't record -o with open/tag/eval/msg
    }

    return true;
}

UNITTEST {
    // NOLINTBEGIN(bugprone-assert-side-effect)
    const char *args[4] = {NULL};
    char **argp = (char**)args;
    const Command *cmd = find_normal_command("left");
    BUG_ON(!cmd);
    BUG_ON(!allow_macro_recording(cmd, argp));

    cmd = find_normal_command("exec");
    BUG_ON(!cmd);
    BUG_ON(!allow_macro_recording(cmd, argp));

    cmd = find_normal_command("command");
    BUG_ON(!cmd);
    BUG_ON(allow_macro_recording(cmd, argp));

    cmd = find_normal_command("macro");
    BUG_ON(!cmd);
    BUG_ON(allow_macro_recording(cmd, argp));

    cmd = find_normal_command("search");
    BUG_ON(!cmd);
    BUG_ON(allow_macro_recording(cmd, argp));
    args[0] = "xyz";
    BUG_ON(!allow_macro_recording(cmd, argp));
    args[0] = "-n";
    BUG_ON(!allow_macro_recording(cmd, argp));
    args[0] = "-p";
    BUG_ON(!allow_macro_recording(cmd, argp));
    args[0] = "-w";
    BUG_ON(!allow_macro_recording(cmd, argp));
    args[0] = "-Hr";
    BUG_ON(allow_macro_recording(cmd, argp));
    args[1] = "str";
    BUG_ON(!allow_macro_recording(cmd, argp));
    // NOLINTEND(bugprone-assert-side-effect)
}

static void record_command(EditorState *e, const Command *cmd, char **args)
{
    if (!allow_macro_recording(cmd, args)) {
        return;
    }
    macro_command_hook(&e->macro, cmd->name, args);
}

const Command *find_normal_command(const char *name)
{
    return BSEARCH(name, cmds, command_cmp);
}

const CommandSet normal_commands = {
    .lookup = find_normal_command,
    .macro_record = record_command,
};

const char *find_normal_alias(const EditorState *e, const char *name)
{
    return find_alias(&e->aliases, name);
}

bool handle_normal_command(EditorState *e, const char *cmd, bool allow_recording)
{
    CommandRunner runner = normal_mode_cmdrunner(e);
    runner.allow_recording = allow_recording;
    return handle_command(&runner, cmd);
}

void exec_normal_config(EditorState *e, StringView config)
{
    CommandRunner runner = normal_mode_cmdrunner(e);
    exec_config(&runner, config);
}

int read_normal_config(EditorState *e, const char *filename, ConfigFlags flags)
{
    CommandRunner runner = normal_mode_cmdrunner(e);
    return read_config(&runner, filename, flags);
}

void collect_normal_commands(PointerArray *a, const char *prefix)
{
    COLLECT_STRING_FIELDS(cmds, name, a, prefix);
}

UNITTEST {
    CHECK_BSEARCH_ARRAY(cmds, name, strcmp);

    for (size_t i = 0, n = ARRAYLEN(cmds); i < n; i++) {
        // Check that flags arrays is null-terminated within bounds
        const char *const flags = cmds[i].flags;
        BUG_ON(flags[ARRAYLEN(cmds[0].flags) - 1] != '\0');

        // Count number of real flags (i.e. not including '=')
        size_t nr_real_flags = 0;
        for (size_t j = 0; flags[j]; j++) {
            unsigned char flag = flags[j];
            if (ascii_isalnum(flag)) {
                nr_real_flags++;
            } else if (flag != '=') {
                BUG("invalid command flag: 0x%02hhX", flag);
            }
        }

        // Check that max. number of real flags fits in CommandArgs::flags
        // array (and also leaves 1 byte for null-terminator)
        CommandArgs a;
        BUG_ON(nr_real_flags >= ARRAYLEN(a.flags));
    }
}
