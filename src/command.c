#include <errno.h>
#include <glob.h>
#include <unistd.h>
#include "alias.h"
#include "bind.h"
#include "change.h"
#include "cmdline.h"
#include "command.h"
#include "config.h"
#include "debug.h"
#include "edit.h"
#include "editor.h"
#include "encoding/convert.h"
#include "encoding/encoding.h"
#include "error.h"
#include "file-option.h"
#include "filetype.h"
#include "frame.h"
#include "history.h"
#include "load-save.h"
#include "lock.h"
#include "move.h"
#include "msg.h"
#include "parse-args.h"
#include "search.h"
#include "selection.h"
#include "spawn.h"
#include "syntax/state.h"
#include "syntax/syntax.h"
#include "tag.h"
#include "terminal/color.h"
#include "terminal/input.h"
#include "terminal/terminal.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"
#include "util/xsnprintf.h"
#include "view.h"
#include "window.h"

static void do_selection(SelectionType sel)
{
    if (sel == SELECT_NONE) {
        if (view->next_movement_cancels_selection) {
            view->next_movement_cancels_selection = false;
            unselect();
        }
        return;
    }

    view->next_movement_cancels_selection = true;

    if (view->selection) {
        view->selection = sel;
        mark_all_lines_changed(buffer);
        return;
    }

    view->sel_so = block_iter_get_offset(&view->cursor);
    view->sel_eo = UINT_MAX;
    view->selection = sel;

    // Need to mark current line changed because cursor might
    // move up or down before screen is updated
    view_update_cursor_y(view);
    buffer_mark_lines_changed(view->buffer, view->cy, view->cy);
}

static void handle_select_chars_flag(const char *pf)
{
    do_selection(strchr(pf, 'c') ? SELECT_CHARS : SELECT_NONE);
}

static void handle_select_chars_or_lines_flags(const char *pf)
{
    SelectionType sel = SELECT_NONE;
    if (strchr(pf, 'l')) {
        sel = SELECT_LINES;
    } else if (strchr(pf, 'c')) {
        sel = SELECT_CHARS;
    }
    do_selection(sel);
}

// Go to compiler error saving position if file changed or cursor moved
static void activate_current_message_save(void)
{
    FileLocation *loc = file_location_create (
        view->buffer->abs_filename,
        view->buffer->id,
        view->cy + 1,
        view->cx_char + 1
    );

    BlockIter save = view->cursor;

    activate_current_message();
    if (view->cursor.blk != save.blk || view->cursor.offset != save.offset) {
        push_file_location(loc);
    } else {
        file_location_free(loc);
    }
}

static void cmd_alias(const CommandArgs *a)
{
    add_alias(a->args[0], a->args[1]);
}

static void cmd_bind(const CommandArgs *a)
{
    const char *key = a->args[0];
    const char *cmd = a->args[1];
    if (cmd) {
        add_binding(key, cmd);
    } else {
        remove_binding(key);
    }
}

static void cmd_bof(const CommandArgs* UNUSED_ARG(a))
{
    move_bof();
}

static void cmd_bol(const CommandArgs *a)
{
    handle_select_chars_flag(a->flags);
    if (strchr(a->flags, 's')) {
        move_bol_smart();
    } else {
        move_bol();
    }
}

static void cmd_bolsf(const CommandArgs* UNUSED_ARG(a))
{
    do_selection(SELECT_NONE);
    if (!block_iter_bol(&view->cursor)) {
        long top = view->vy + window_get_scroll_margin(window);
        if (view->cy > top) {
            move_up(view->cy - top);
        } else {
            block_iter_bof(&view->cursor);
        }
    }
    view_reset_preferred_x(view);
}

static void cmd_case(const CommandArgs *a)
{
    const char *pf = a->flags;
    int mode = 't';
    while (*pf) {
        switch (*pf) {
        case 'l':
        case 'u':
            mode = *pf;
            break;
        }
        pf++;
    }
    change_case(mode);
}

static void cmd_cd(const CommandArgs *a)
{
    const char *dir = a->args[0];
    char cwd[8192];
    const char *cwdp = NULL;
    bool got_cwd = !!getcwd(cwd, sizeof(cwd));

    if (streq(dir, "-")) {
        dir = getenv("OLDPWD");
        if (dir == NULL || dir[0] == '\0') {
            error_msg("cd: OLDPWD not set");
            return;
        }
    }
    if (chdir(dir)) {
        error_msg("cd: %s", strerror(errno));
        return;
    }

    if (got_cwd) {
        setenv("OLDPWD", cwd, 1);
    }
    got_cwd = !!getcwd(cwd, sizeof(cwd));
    if (got_cwd) {
        setenv("PWD", cwd, 1);
        cwdp = cwd;
    }

    for (size_t i = 0; i < buffers.count; i++) {
        Buffer *b = buffers.ptrs[i];
        update_short_filename_cwd(b, cwdp);
    }

    // Need to update all tabbars
    mark_everything_changed();
}

static void cmd_center_view(const CommandArgs* UNUSED_ARG(a))
{
    view->force_center = true;
}

static void cmd_clear(const CommandArgs* UNUSED_ARG(a))
{
    clear_lines();
}

static void cmd_close(const CommandArgs *a)
{
    const char *pf = a->flags;
    bool force = false;
    bool allow_quit = false;
    bool allow_wclose = false;
    while (*pf) {
        switch (*pf) {
        case 'f':
            force = true;
            break;
        case 'q':
            allow_quit = true;
            break;
        case 'w':
            allow_wclose = true;
            break;
        }
        pf++;
    }

    if (!view_can_close(view) && !force) {
        error_msg (
            "The buffer is modified. "
            "Save or run 'close -f' to close without saving."
        );
        return;
    }

    if (allow_quit && buffers.count == 1 && root_frame->frames.count <= 1) {
        editor.status = EDITOR_EXITING;
        return;
    }

    if (allow_wclose && window->views.count <= 1) {
        window_close_current();
        return;
    }

    window_close_current_view(window);
    set_view(window->view);
}

static void cmd_command(const CommandArgs *a)
{
    const char *text = a->args[0];
    set_input_mode(INPUT_COMMAND);
    if (text) {
        cmdline_set_text(&editor.cmdline, text);
    }
}

static void cmd_compile(const CommandArgs *a)
{
    const char *pf = a->flags;
    SpawnFlags flags = SPAWN_DEFAULT;
    while (*pf) {
        switch (*pf) {
        case '1':
            flags |= SPAWN_READ_STDOUT;
            break;
        case 'p':
            flags |= SPAWN_PROMPT;
            break;
        case 's':
            flags |= SPAWN_QUIET;
            break;
        }
        pf++;
    }

    const char *name = a->args[0];
    Compiler *c = find_compiler(name);
    if (!c) {
        error_msg("No such error parser %s", name);
        return;
    }
    clear_messages();
    spawn_compiler(a->args + 1, flags, c);
    if (message_count()) {
        activate_current_message_save();
    }
}

static void cmd_copy(const CommandArgs *a)
{
    BlockIter save = view->cursor;
    if (view->selection) {
        copy(prepare_selection(view), view->selection == SELECT_LINES);
        bool keep_selection = a->flags[0] == 'k';
        if (!keep_selection) {
            unselect();
        }
    } else {
        block_iter_bol(&view->cursor);
        BlockIter tmp = view->cursor;
        copy(block_iter_eat_line(&tmp), true);
    }
    view->cursor = save;
}

static void cmd_cut(const CommandArgs* UNUSED_ARG(a))
{
    const long x = view_get_preferred_x(view);
    if (view->selection) {
        cut(prepare_selection(view), view->selection == SELECT_LINES);
        if (view->selection == SELECT_LINES) {
            move_to_preferred_x(x);
        }
        unselect();
    } else {
        BlockIter tmp;
        block_iter_bol(&view->cursor);
        tmp = view->cursor;
        cut(block_iter_eat_line(&tmp), true);
        move_to_preferred_x(x);
    }
}

static void cmd_delete(const CommandArgs* UNUSED_ARG(a))
{
    delete_ch();
}

static void cmd_delete_eol(const CommandArgs *a)
{
    if (view->selection) {
        return;
    }

    bool delete_newline_if_at_eol = a->flags[0] == 'n';
    BlockIter bi = view->cursor;
    if (delete_newline_if_at_eol) {
        CodePoint ch;
        block_iter_get_char(&view->cursor, &ch);
        if (ch == '\n') {
            delete_ch();
            return;
        }
    }

    buffer_delete_bytes(block_iter_eol(&bi));
}

static void cmd_delete_word(const CommandArgs *a)
{
    bool skip_non_word = a->flags[0] == 's';
    BlockIter bi = view->cursor;
    buffer_delete_bytes(word_fwd(&bi, skip_non_word));
}

static void cmd_down(const CommandArgs *a)
{
    handle_select_chars_or_lines_flags(a->flags);
    move_down(1);
}

static void cmd_eof(const CommandArgs* UNUSED_ARG(a))
{
    move_eof();
}

static void cmd_eol(const CommandArgs *a)
{
    handle_select_chars_flag(a->flags);
    move_eol();
}

static void cmd_eolsf(const CommandArgs* UNUSED_ARG(a))
{
    do_selection(SELECT_NONE);
    if (!block_iter_eol(&view->cursor)) {
        long bottom = view->vy + window->edit_h - 1 - window_get_scroll_margin(window);
        if (view->cy < bottom) {
            move_down(bottom - view->cy);
        } else {
            block_iter_eof(&view->cursor);
        }
    }
    view_reset_preferred_x(view);
}

static void cmd_erase(const CommandArgs* UNUSED_ARG(a))
{
    erase();
}

static void cmd_erase_bol(const CommandArgs* UNUSED_ARG(a))
{
    buffer_erase_bytes(block_iter_bol(&view->cursor));
}

static void cmd_erase_word(const CommandArgs *a)
{
    bool skip_non_word = a->flags[0] == 's';
    buffer_erase_bytes(word_bwd(&view->cursor, skip_non_word));
}

static void cmd_errorfmt(const CommandArgs *a)
{
    bool ignore = a->flags[0] == 'i';
    add_error_fmt(a->args[0], ignore, a->args[1], a->args + 2);
}

static void cmd_eval(const CommandArgs *a)
{
    FilterData data = FILTER_DATA_INIT;
    if (spawn_filter(a->args, &data)) {
        return;
    }
    exec_config(commands, data.out, data.out_len);
    free(data.out);
}

static void cmd_filter(const CommandArgs *a)
{
    FilterData data;
    BlockIter save = view->cursor;

    if (view->selection) {
        data.in_len = prepare_selection(view);
    } else {
        Block *blk;
        data.in_len = 0;
        block_for_each(blk, &buffer->blocks) {
            data.in_len += blk->size;
        }
        move_bof();
    }

    data.in = block_iter_get_bytes(&view->cursor, data.in_len);
    if (spawn_filter(a->args, &data)) {
        free(data.in);
        view->cursor = save;
        return;
    }

    free(data.in);
    buffer_replace_bytes(data.in_len, data.out, data.out_len);
    free(data.out);

    unselect();
}

static void cmd_ft(const CommandArgs *a)
{
    const char *pf = a->flags;
    FileDetectionType dt = FT_EXTENSION;
    while (*pf) {
        switch (*pf) {
        case 'b':
            dt = FT_BASENAME;
            break;
        case 'c':
            dt = FT_CONTENT;
            break;
        case 'f':
            dt = FT_FILENAME;
            break;
        case 'i':
            dt = FT_INTERPRETER;
            break;
        }
        pf++;
    }

    char **args = a->args;
    if (args[0][0] == '\0') {
        error_msg("Filetype can't be blank");
        return;
    }

    for (size_t i = 1; args[i]; i++) {
        add_filetype(args[0], args[i], dt);
    }
}

static void cmd_git_open(const CommandArgs* UNUSED_ARG(a))
{
    set_input_mode(INPUT_GIT_OPEN);
    git_open_reload();
}

static void cmd_hi(const CommandArgs *a)
{
    char **args = a->args;
    TermColor color;
    if (args[0] == NULL) {
        exec_reset_colors_rc();
        remove_extra_colors();
    } else if (parse_term_color(&color, args + 1)) {
        color.fg = color_to_nearest(color.fg, terminal.color_type);
        color.bg = color_to_nearest(color.bg, terminal.color_type);
        set_highlight_color(args[0], &color);
    }

    // Don't call update_all_syntax_colors() needlessly.
    // It is called right after config has been loaded.
    if (editor.status != EDITOR_INITIALIZING) {
        update_all_syntax_colors();
        mark_everything_changed();
    }
}

static void cmd_include(const CommandArgs *a)
{
    ConfigFlags flags = CFG_MUST_EXIST;
    if (a->flags[0] == 'b') {
        flags |= CFG_BUILTIN;
    }
    read_config(commands, a->args[0], flags);
}

static void cmd_insert(const CommandArgs *a)
{
    const char *str = a->args[0];
    if (strchr(a->flags, 'k')) {
        for (size_t i = 0; str[i]; i++) {
            insert_ch(str[i]);
        }
        return;
    }

    size_t del_len = 0;
    size_t ins_len = strlen(str);
    if (view->selection) {
        del_len = prepare_selection(view);
        unselect();
    }
    buffer_replace_bytes(del_len, str, ins_len);

    if (strchr(a->flags, 'm')) {
        block_iter_skip_bytes(&view->cursor, ins_len);
    }
}

static void cmd_insert_builtin(const CommandArgs *a)
{
    const char *name = a->args[0];
    const BuiltinConfig *cfg = get_builtin_config(name);
    if (cfg) {
        buffer_insert_bytes(cfg->text.data, cfg->text.length);
    } else {
        error_msg("No built-in config with name '%s'", name);
    }
}

static void cmd_join(const CommandArgs* UNUSED_ARG(a))
{
    join_lines();
}

static void cmd_left(const CommandArgs *a)
{
    handle_select_chars_flag(a->flags);
    move_cursor_left();
}

static void cmd_line(const CommandArgs *a)
{
    const char *arg = a->args[0];
    const long x = view_get_preferred_x(view);
    size_t line;
    if (!str_to_size(arg, &line) || line == 0) {
        error_msg("Invalid line number: %s", arg);
        return;
    }
    move_to_line(view, line);
    move_to_preferred_x(x);
}

static void cmd_load_syntax(const CommandArgs *a)
{
    const char *filename = a->args[0];
    const char *filetype = path_basename(filename);
    if (filename != filetype) {
        if (find_syntax(filetype)) {
            error_msg("Syntax for filetype %s already loaded", filetype);
        } else {
            int err;
            load_syntax_file(filename, true, &err);
        }
    } else {
        if (!find_syntax(filetype)) {
            load_syntax_by_filetype(filetype);
        }
    }
}

static void cmd_move_tab(const CommandArgs *a)
{
    const char *str = a->args[0];
    size_t j, i = ptr_array_idx(&window->views, view);
    if (streq(str, "left")) {
        j = i - 1;
    } else if (streq(str, "right")) {
        j = i + 1;
    } else {
        size_t num;
        if (!str_to_size(str, &num) || num == 0) {
            error_msg("Invalid tab position %s", str);
            return;
        }
        j = num - 1;
        if (j >= window->views.count) {
            j = window->views.count - 1;
        }
    }
    j = (window->views.count + j) % window->views.count;
    ptr_array_insert (
        &window->views,
        ptr_array_remove_idx(&window->views, i),
        j
    );
    window->update_tabbar = true;
}

static void cmd_msg(const CommandArgs *a)
{
    const char *pf = a->flags;
    char dir = 0;
    while (*pf) {
        switch (*pf) {
        case 'n':
        case 'p':
            dir = *pf;
            break;
        }
        pf++;
    }

    if (dir == 'n') {
        activate_next_message();
    } else if (dir == 'p') {
        activate_prev_message();
    } else {
        activate_current_message();
    }
}

static void cmd_new_line(const CommandArgs* UNUSED_ARG(a))
{
    new_line();
}

static void cmd_next(const CommandArgs* UNUSED_ARG(a))
{
    set_view(ptr_array_next(&window->views, view));
}

static void cmd_open(const CommandArgs *a)
{
    char **args = a->args;
    const char *pf = a->flags;
    const char *requested_encoding = NULL;
    bool use_glob = false;
    while (*pf) {
        switch (*pf) {
        case 'e':
            requested_encoding = *args++;
            break;
        case 'g':
            use_glob = args[0] ? true : false;
            break;
        }
        pf++;
    }

    Encoding encoding = {
        .type = ENCODING_AUTODETECT,
        .name = NULL
    };

    if (requested_encoding) {
        if (
            lookup_encoding(requested_encoding) != UTF8
            && !encoding_supported_by_iconv(requested_encoding)
        ) {
            error_msg("Unsupported encoding %s", requested_encoding);
            return;
        }
        encoding = encoding_from_name(requested_encoding);
    }

    char **paths = args;
    glob_t globbuf;
    if (use_glob) {
        int err = glob(args[0], GLOB_NOCHECK, NULL, &globbuf);
        while (err == 0 && *++args) {
            err = glob(*args, GLOB_NOCHECK | GLOB_APPEND, NULL, &globbuf);
        }
        if (globbuf.gl_pathc > 0) {
            paths = globbuf.gl_pathv;
        }
    }

    if (!paths[0]) {
        window_open_new_file(window);
        if (requested_encoding) {
            buffer->encoding = encoding;
        }
    } else if (!paths[1]) {
        // Previous view is remembered when opening single file
        window_open_file(window, paths[0], &encoding);
    } else {
        // It makes no sense to remember previous view when opening
        // multiple files
        window_open_files(window, paths, &encoding);
    }

    if (use_glob) {
        globfree(&globbuf);
    }
}

static void cmd_option(const CommandArgs *a)
{
    char **args = a->args;
    size_t argc = a->nr_args;
    char **strs = args + 1;
    size_t count = argc - 1;

    if (argc % 2 == 0) {
        error_msg("Missing option value");
        return;
    }
    if (!validate_local_options(strs)) {
        return;
    }

    if (a->flags[0] == 'r') {
        add_file_options (
            FILE_OPTIONS_FILENAME,
            xstrdup(args[0]),
            copy_string_array(strs, count)
        );
        return;
    }

    char *comma, *list = args[0];
    do {
        comma = strchr(list, ',');
        size_t len = comma ? comma - list : strlen(list);
        add_file_options (
            FILE_OPTIONS_FILETYPE,
            xstrcut(list, len),
            copy_string_array(strs, count)
        );
        list = comma + 1;
    } while (comma);
}

static void cmd_paste(const CommandArgs *a)
{
    bool at_cursor = a->flags[0] == 'c';
    paste(at_cursor);
}

static void cmd_pgdown(const CommandArgs *a)
{
    handle_select_chars_or_lines_flags(a->flags);

    long margin = window_get_scroll_margin(window);
    long bottom = view->vy + window->edit_h - 1 - margin;
    long count;

    if (view->cy < bottom) {
        count = bottom - view->cy;
    } else {
        count = window->edit_h - 1 - margin * 2;
    }
    move_down(count);
}

static void cmd_pgup(const CommandArgs *a)
{
    handle_select_chars_or_lines_flags(a->flags);

    long margin = window_get_scroll_margin(window);
    long top = view->vy + margin;
    long count;

    if (view->cy > top) {
        count = view->cy - top;
    } else {
        count = window->edit_h - 1 - margin * 2;
    }
    move_up(count);
}

static void cmd_pipe_from(const CommandArgs *a)
{
    const char *pf = a->flags;
    bool strip_nl = false;
    bool move = false;
    while (*pf) {
        switch (*pf++) {
        case 'm':
            move = true;
            break;
        case 's':
            strip_nl = true;
            break;
        }
    }

    FilterData data = FILTER_DATA_INIT;
    if (spawn_filter(a->args, &data)) {
        return;
    }

    size_t del_len = 0;
    if (view->selection) {
        del_len = prepare_selection(view);
        unselect();
    }

    if (strip_nl && data.out_len > 0 && data.out[data.out_len - 1] == '\n') {
        if (--data.out_len > 0 && data.out[data.out_len - 1] == '\r') {
            data.out_len--;
        }
    }

    buffer_replace_bytes(del_len, data.out, data.out_len);
    free(data.out);

    if (move) {
        block_iter_skip_bytes(&view->cursor, data.out_len);
    }
}

static void cmd_pipe_to(const CommandArgs *a)
{
    const BlockIter saved_cursor = view->cursor;
    const ssize_t saved_sel_so = view->sel_so;
    const ssize_t saved_sel_eo = view->sel_eo;

    size_t input_len = 0;
    if (view->selection) {
        input_len = prepare_selection(view);
    } else {
        Block *blk;
        block_for_each(blk, &buffer->blocks) {
            input_len += blk->size;
        }
        move_bof();
    }

    char *input = block_iter_get_bytes(&view->cursor, input_len);
    spawn_writer(a->args, input, input_len);
    free(input);

    // Restore cursor and selection offsets, instead of calling unselect()
    view->cursor = saved_cursor;
    view->sel_so = saved_sel_so;
    view->sel_eo = saved_sel_eo;
}

static void cmd_prev(const CommandArgs* UNUSED_ARG(a))
{
    set_view(ptr_array_prev(&window->views, view));
}

static void cmd_quit(const CommandArgs *a)
{
    const char *pf = a->flags;
    bool prompt = false;
    while (*pf) {
        switch (*pf++) {
        case 'f':
            editor.status = EDITOR_EXITING;
            return;
        case 'p':
            prompt = true;
            break;
        }
    }

    for (size_t i = 0; i < buffers.count; i++) {
        Buffer *b = buffers.ptrs[i];
        if (buffer_modified(b)) {
            // Activate modified buffer
            View *v = window_find_view(window, b);

            if (v == NULL) {
                // Buffer isn't open in current window.
                // Activate first window of the buffer.
                v = b->views.ptrs[0];
                window = v->window;
                mark_everything_changed();
            }
            set_view(v);
            if (prompt) {
                if (get_confirmation("yN", "Quit without saving changes?") == 'y') {
                    editor.status = EDITOR_EXITING;
                }
                return;
            } else {
                error_msg (
                    "Save modified files or run 'quit -f' to quit"
                    " without saving."
                );
                return;
            }
        }
    }

    editor.status = EDITOR_EXITING;
}

static void cmd_redo(const CommandArgs *a)
{
    char *arg = a->args[0];
    unsigned long change_id = 0;
    if (arg) {
        if (!str_to_ulong(arg, &change_id) || change_id == 0) {
            error_msg("Invalid change id: %s", arg);
            return;
        }
    }
    if (redo(change_id)) {
        unselect();
    }
}

static void cmd_refresh(const CommandArgs* UNUSED_ARG(a))
{
    mark_everything_changed();
}

static void cmd_repeat(const CommandArgs *a)
{
    char **args = a->args;
    unsigned int count = 0;
    if (!str_to_uint(args[0], &count)) {
        error_msg("Not a valid repeat count: %s", args[0]);
        return;
    } else if (count == 0) {
        return;
    }

    const Command *cmd = find_command(commands, args[1]);
    if (!cmd) {
        error_msg("No such command: %s", args[1]);
        return;
    }

    CommandArgs a2 = {.args = args + 2};
    if (parse_args(cmd, &a2)) {
        while (count-- > 0) {
            cmd->cmd(&a2);
        }
    }
}

static void cmd_replace(const CommandArgs *a)
{
    const char *pf = a->flags;
    unsigned int flags = 0;
    for (size_t i = 0; pf[i]; i++) {
        switch (pf[i]) {
        case 'b':
            flags |= REPLACE_BASIC;
            break;
        case 'c':
            flags |= REPLACE_CONFIRM;
            break;
        case 'g':
            flags |= REPLACE_GLOBAL;
            break;
        case 'i':
            flags |= REPLACE_IGNORE_CASE;
            break;
        }
    }
    reg_replace(a->args[0], a->args[1], flags);
}

static void cmd_right(const CommandArgs *a)
{
    handle_select_chars_flag(a->flags);
    move_cursor_right();
}

static void cmd_run(const CommandArgs *a)
{
    const char *pf = a->flags;
    int fd[3] = {0, 1, 2};
    bool prompt = false;
    while (*pf) {
        switch (*pf) {
        case 'p':
            prompt = true;
            break;
        case 's':
            fd[0] = -1;
            fd[1] = -1;
            fd[2] = -1;
            break;
        }
        pf++;
    }
    spawn(a->args, fd, prompt);
}

static bool stat_changed(const struct stat *const a, const struct stat *const b)
{
    // Don't compare st_mode because we allow chmod 755 etc.
    return a->st_mtime != b->st_mtime ||
        a->st_dev != b->st_dev ||
        a->st_ino != b->st_ino;
}

static void cmd_save(const CommandArgs *a)
{
    const char *pf = a->flags;
    char **args = a->args;
    char *absolute = buffer->abs_filename;
    Encoding encoding = buffer->encoding;
    const char *requested_encoding = NULL;
    bool force = false;
    bool prompt = false;
    LineEndingType newline = buffer->newline;
    mode_t old_mode = buffer->st.st_mode;
    struct stat st;
    bool new_locked = false;

    while (*pf) {
        switch (*pf) {
        case 'd':
            newline = NEWLINE_DOS;
            break;
        case 'e':
            requested_encoding = *args++;
            break;
        case 'f':
            force = true;
            break;
        case 'p':
            prompt = true;
            break;
        case 'u':
            newline = NEWLINE_UNIX;
            break;
        }
        pf++;
    }

    if (requested_encoding) {
        if (
            lookup_encoding(requested_encoding) != UTF8
            && !encoding_supported_by_iconv(requested_encoding)
        ) {
            error_msg("Unsupported encoding %s", requested_encoding);
            return;
        }
        encoding = encoding_from_name(requested_encoding);
    }

    // The encoding_from_name() call above may have allocated memory,
    // so use "goto error" instead of early return beyond this point, to
    // ensure correct de-allocation.

    if (args[0]) {
        if (args[0][0] == '\0') {
            error_msg("Empty filename not allowed");
            goto error;
        }
        char *tmp = path_absolute(args[0]);
        if (!tmp) {
            error_msg("Failed to make absolute path: %s", strerror(errno));
            goto error;
        }
        if (absolute && streq(tmp, absolute)) {
            free(tmp);
        } else {
            absolute = tmp;
        }
    } else {
        if (!absolute) {
            if (prompt) {
                set_input_mode(INPUT_COMMAND);
                cmdline_set_text(&editor.cmdline, "save ");
                // This branch is not really an error, but we still return via
                // the "error" label because we need to clean up memory and
                // that's all it's used for currently.
                goto error;
            } else {
                error_msg("No filename.");
                goto error;
            }
        }
        if (buffer->readonly && !force) {
            error_msg("Use -f to force saving read-only file.");
            goto error;
        }
    }

    if (stat(absolute, &st)) {
        if (errno != ENOENT) {
            error_msg("stat failed for %s: %s", absolute, strerror(errno));
            goto error;
        }
        if (editor.options.lock_files) {
            if (absolute == buffer->abs_filename) {
                if (!buffer->locked) {
                    if (lock_file(absolute)) {
                        if (!force) {
                            error_msg("Can't lock file %s", absolute);
                            goto error;
                        }
                    } else {
                        buffer->locked = true;
                    }
                }
            } else {
                if (lock_file(absolute)) {
                    if (!force) {
                        error_msg("Can't lock file %s", absolute);
                        goto error;
                    }
                } else {
                    new_locked = true;
                }
            }
        }
    } else {
        if (
            absolute == buffer->abs_filename
            && !force
            && stat_changed(&buffer->st, &st)
        ) {
            error_msg (
                "File has been modified by someone else."
                " Use -f to force overwrite."
            );
            goto error;
        }
        if (S_ISDIR(st.st_mode)) {
            error_msg("Will not overwrite directory %s", absolute);
            goto error;
        }
        if (editor.options.lock_files) {
            if (absolute == buffer->abs_filename) {
                if (!buffer->locked) {
                    if (lock_file(absolute)) {
                        if (!force) {
                            error_msg("Can't lock file %s", absolute);
                            goto error;
                        }
                    } else {
                        buffer->locked = true;
                    }
                }
            } else {
                if (lock_file(absolute)) {
                    if (!force) {
                        error_msg("Can't lock file %s", absolute);
                        goto error;
                    }
                } else {
                    new_locked = true;
                }
            }
        }
        if (absolute != buffer->abs_filename && !force) {
            error_msg("Use -f to overwrite %s.", absolute);
            goto error;
        }

        // Allow chmod 755 etc.
        buffer->st.st_mode = st.st_mode;
    }
    if (save_buffer(buffer, absolute, &encoding, newline)) {
        goto error;
    }

    buffer->saved_change = buffer->cur_change;
    buffer->readonly = false;
    buffer->newline = newline;
    if (requested_encoding) {
        buffer->encoding = encoding;
    }

    if (absolute != buffer->abs_filename) {
        if (buffer->locked) {
            // Filename changes, release old file lock
            unlock_file(buffer->abs_filename);
        }
        buffer->locked = new_locked;

        free(buffer->abs_filename);
        buffer->abs_filename = absolute;
        update_short_filename(buffer);

        // Filename change is not detected (only buffer_modified() change)
        mark_buffer_tabbars_changed(buffer);
    }
    if (!old_mode && streq(buffer->options.filetype, "none")) {
        // New file and most likely user has not changed the filetype
        if (buffer_detect_filetype(buffer)) {
            set_file_options(buffer);
            set_editorconfig_options(buffer);
            buffer_update_syntax(buffer);
        }
    }
    return;
error:
    if (new_locked) {
        unlock_file(absolute);
    }
    if (absolute != buffer->abs_filename) {
        free(absolute);
    }
}

static void cmd_scroll_down(const CommandArgs* UNUSED_ARG(a))
{
    view->vy++;
    if (view->cy < view->vy) {
        move_down(1);
    }
}

static void cmd_scroll_pgdown(const CommandArgs* UNUSED_ARG(a))
{
    long max = buffer->nl - window->edit_h + 1;
    if (view->vy < max && max > 0) {
        long count = window->edit_h - 1;
        if (view->vy + count > max) {
            count = max - view->vy;
        }
        view->vy += count;
        move_down(count);
    } else if (view->cy < buffer->nl) {
        move_down(buffer->nl - view->cy);
    }
}

static void cmd_scroll_pgup(const CommandArgs* UNUSED_ARG(a))
{
    if (view->vy > 0) {
        long count = window->edit_h - 1;
        if (count > view->vy) {
            count = view->vy;
        }
        view->vy -= count;
        move_up(count);
    } else if (view->cy > 0) {
        move_up(view->cy);
    }
}

static void cmd_scroll_up(const CommandArgs* UNUSED_ARG(a))
{
    if (view->vy) {
        view->vy--;
    }
    if (view->vy + window->edit_h <= view->cy) {
        move_up(1);
    }
}

static void cmd_search(const CommandArgs *a)
{
    const char *pf = a->flags;
    char *pattern = a->args[0];
    bool history = true;
    char cmd = 0;
    bool w = false;
    SearchDirection dir = SEARCH_FWD;

    while (*pf) {
        switch (*pf) {
        case 'H':
            history = false;
            break;
        case 'n':
        case 'p':
            cmd = *pf;
            break;
        case 'r':
            dir = SEARCH_BWD;
            break;
        case 'w':
            w = true;
            if (pattern) {
                error_msg("Flag -w can't be used with search pattern.");
                return;
            }
            break;
        }
        pf++;
    }

    if (w) {
        char *word = view_get_word_under_cursor(view);
        if (word == NULL) {
            // Error message would not be very useful here
            return;
        }
        size_t len = strlen(word) + 5;
        pattern = xmalloc(len);
        xsnprintf(pattern, len, "\\<%s\\>", word);
        free(word);
    }

    if (pattern) {
        search_set_direction(dir);
        search_set_regexp(pattern);
        if (w) {
            search_next_word();
        } else {
            search_next();
        }
        if (history) {
            history_add(&editor.search_history, pattern, search_history_size);
        }

        if (pattern != a->args[0]) {
            free(pattern);
        }
    } else if (cmd == 'n') {
        search_next();
    } else if (cmd == 'p') {
        search_prev();
    } else {
        set_input_mode(INPUT_SEARCH);
        search_set_direction(dir);
    }
}

static void cmd_select(const CommandArgs *a)
{
    const char *pf = a->flags;
    SelectionType sel = SELECT_CHARS;
    bool block = false;
    bool keep = false;

    while (*pf) {
        switch (*pf) {
        case 'b':
            block = true;
            break;
        case 'k':
            keep = true;
            break;
        case 'l':
            block = false;
            sel = SELECT_LINES;
            break;
        }
        pf++;
    }

    view->next_movement_cancels_selection = false;

    if (block) {
        select_block();
        return;
    }

    if (view->selection) {
        if (!keep && view->selection == sel) {
            unselect();
            return;
        }
        view->selection = sel;
        mark_all_lines_changed(buffer);
        return;
    }

    view->sel_so = block_iter_get_offset(&view->cursor);
    view->sel_eo = UINT_MAX;
    view->selection = sel;

    // Need to mark current line changed because cursor might
    // move up or down before screen is updated
    view_update_cursor_y(view);
    buffer_mark_lines_changed(view->buffer, view->cy, view->cy);
}

static void cmd_set(const CommandArgs *a)
{
    const char *pf = a->flags;
    bool global = false;
    bool local = false;
    while (*pf) {
        switch (*pf) {
        case 'g':
            global = true;
            break;
        case 'l':
            local = true;
            break;
        }
        pf++;
    }

    // You can set only global values in config file
    if (buffer == NULL) {
        if (local) {
            error_msg("Flag -l makes no sense in config file.");
            return;
        }
        global = true;
    }

    char **args = a->args;
    size_t count = a->nr_args;
    if (count == 1) {
        set_bool_option(args[0], local, global);
        return;
    } else if (count % 2 != 0) {
        error_msg("One or even number of arguments expected.");
        return;
    }

    for (size_t i = 0; i < count; i += 2) {
        set_option(args[i], args[i + 1], local, global);
    }
}

static void cmd_setenv(const CommandArgs *a)
{
    char **args = a->args;
    if (setenv(args[0], args[1], 1) < 0) {
        switch (errno) {
        case EINVAL:
            error_msg("Invalid environment variable name '%s'", args[0]);
            break;
        default:
            error_msg("%s", strerror(errno));
        }
    }
}

static void cmd_shift(const CommandArgs *a)
{
    const char *arg = a->args[0];
    int count;
    if (!str_to_int(arg, &count)) {
        error_msg("Invalid number: %s", arg);
        return;
    }
    if (count == 0) {
        error_msg("Count must be non-zero.");
        return;
    }
    shift_lines(count);
}

static void show_alias(const char *alias_name, bool write_to_cmdline)
{
    const char *cmd_str = find_alias(alias_name);
    if (cmd_str == NULL) {
        if (find_command(commands, alias_name)) {
            info_msg("%s is a built-in command, not an alias", alias_name);
        } else {
            info_msg("%s is not a known alias", alias_name);
        }
        return;
    }

    if (write_to_cmdline) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, cmd_str);
    } else {
        info_msg("%s is aliased to: %s", alias_name, cmd_str);
    }
}

static void show_binding(const char *keystr, bool write_to_cmdline)
{
    KeyCode key;
    if (!parse_key(&key, keystr)) {
        error_msg("invalid key string: %s", keystr);
        return;
    }

    if (u_is_unicode(key)) {
        info_msg("%s is not a bindable key", keystr);
        return;
    }

    const KeyBinding *b = lookup_binding(key);
    if (b == NULL) {
        info_msg("%s is not bound to a command", keystr);
        return;
    }

    if (write_to_cmdline) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, b->cmd_str);
    } else {
        info_msg("%s is bound to: %s", keystr, b->cmd_str);
    }
}

static void cmd_show(const CommandArgs *a)
{
    typedef enum {
        CMD_ALIAS,
        CMD_BIND,
    } CommandType;

    CommandType cmdtype;
    if (streq(a->args[0], "alias")) {
        cmdtype = CMD_ALIAS;
    } else if (streq(a->args[0], "bind")) {
        cmdtype = CMD_BIND;
    } else {
        error_msg("argument #1 must be: 'alias' or 'bind'");
        return;
    }

    const bool cflag = a->nr_flags != 0;
    if (a->nr_args == 2) {
        const char *str = a->args[1];
        switch (cmdtype) {
        case CMD_ALIAS:
            show_alias(str, cflag);
            break;
        case CMD_BIND:
            show_binding(str, cflag);
            break;
        }
        return;
    }

    if (cflag) {
        error_msg("\"show -c\" requires 2 arguments");
        return;
    }

    char tmp[32] = "/tmp/.dte.XXXXXX";
    int fd = mkstemp(tmp);
    if (fd < 0) {
        error_msg("mkstemp() failed: %s", strerror(errno));
        return;
    }

    String s;
    switch (cmdtype) {
    case CMD_ALIAS:
        s = dump_aliases();
        break;
    case CMD_BIND:
        s = dump_bindings();
        break;
    }

    ssize_t rc = xwrite(fd, s.buffer, s.len);
    int err = errno;
    close(fd);
    string_free(&s);
    if (rc < 0) {
        error_msg("write() failed: %s", strerror(err));
        unlink(tmp);
        return;
    }

    const char *argv[] = {editor.pager, tmp, NULL};
    int child_fds[3] = {0, 1, 2};
    spawn((char**)argv, child_fds, false);
    unlink(tmp);
}

static void cmd_suspend(const CommandArgs* UNUSED_ARG(a))
{
    suspend();
}

static void cmd_tag(const CommandArgs *a)
{
    if (a->flags[0] == 'r') {
        pop_file_location();
        return;
    }

    clear_messages();
    TagFile *tf = load_tag_file();
    if (tf == NULL) {
        error_msg("No tag file.");
        return;
    }

    const char *name = a->args[0];
    char *word = NULL;
    if (!name) {
        word = view_get_word_under_cursor(view);
        if (!word) {
            return;
        }
        name = word;
    }

    PointerArray tags = PTR_ARRAY_INIT;
    // Filename helps to find correct tags
    tag_file_find_tags(tf, buffer->abs_filename, name, &tags);
    if (tags.count == 0) {
        error_msg("Tag %s not found.", name);
    } else {
        for (size_t i = 0; i < tags.count; i++) {
            Tag *t = tags.ptrs[i];
            char buf[512];
            xsnprintf(buf, sizeof(buf), "Tag %s", name);
            Message *m = new_message(buf);
            m->loc = xnew0(FileLocation, 1);
            m->loc->filename = tag_file_get_tag_filename(tf, t);
            if (t->pattern) {
                m->loc->pattern = t->pattern;
                t->pattern = NULL;
            } else {
                m->loc->line = t->line;
            }
            add_message(m);
        }
        free_tags(&tags);
        activate_current_message_save();
    }
    free(word);
}

static void cmd_toggle(const CommandArgs *a)
{
    const char *pf = a->flags;
    bool global = false;
    bool verbose = false;
    while (*pf) {
        switch (*pf) {
        case 'g':
            global = true;
            break;
        case 'v':
            verbose = true;
            break;
        }
        pf++;
    }

    const char *option_name = a->args[0];
    size_t nr_values = a->nr_args - 1;
    if (nr_values) {
        char **values = a->args + 1;
        toggle_option_values(option_name, global, verbose, values, nr_values);
    } else {
        toggle_option(option_name, global, verbose);
    }
}

static void cmd_undo(const CommandArgs* UNUSED_ARG(a))
{
    if (undo()) {
        unselect();
    }
}

static void cmd_unselect(const CommandArgs* UNUSED_ARG(a))
{
    unselect();
}

static void cmd_up(const CommandArgs *a)
{
    handle_select_chars_or_lines_flags(a->flags);
    move_up(1);
}

static void cmd_view(const CommandArgs *a)
{
    BUG_ON(window->views.count == 0);
    const char *arg = a->args[0];
    size_t idx;
    if (streq(arg, "last")) {
        idx = window->views.count - 1;
    } else {
        if (!str_to_size(arg, &idx) || idx == 0) {
            error_msg("Invalid view index: %s", arg);
            return;
        }
        idx--;
        if (idx > window->views.count - 1) {
            idx = window->views.count - 1;
        }
    }
    set_view(window->views.ptrs[idx]);
}

static void cmd_wclose(const CommandArgs *a)
{
    View *v = window_find_unclosable_view(window, view_can_close);
    bool force = a->flags[0] == 'f';
    if (v != NULL && !force) {
        set_view(v);
        error_msg (
            "Save modified files or run 'wclose -f' to close "
            "window without saving."
        );
        return;
    }
    window_close_current();
}

static void cmd_wflip(const CommandArgs* UNUSED_ARG(a))
{
    Frame *f = window->frame;
    if (f->parent == NULL) {
        return;
    }
    f->parent->vertical ^= 1;
    mark_everything_changed();
}

static void cmd_wnext(const CommandArgs* UNUSED_ARG(a))
{
    window = next_window(window);
    set_view(window->view);
    mark_everything_changed();
    debug_frames();
}

static void cmd_word_bwd(const CommandArgs *a)
{
    handle_select_chars_flag(a->flags);
    bool skip_non_word = strchr(a->flags, 's');
    word_bwd(&view->cursor, skip_non_word);
    view_reset_preferred_x(view);
}

static void cmd_word_fwd(const CommandArgs *a)
{
    handle_select_chars_flag(a->flags);
    bool skip_non_word = strchr(a->flags, 's');
    word_fwd(&view->cursor, skip_non_word);
    view_reset_preferred_x(view);
}

static void cmd_wprev(const CommandArgs* UNUSED_ARG(a))
{
    window = prev_window(window);
    set_view(window->view);
    mark_everything_changed();
    debug_frames();
}

static void cmd_wrap_paragraph(const CommandArgs *a)
{
    const char *arg = a->args[0];
    size_t width = (size_t)buffer->options.text_width;
    if (arg) {
        if (!str_to_size(arg, &width) || width == 0 || width > 1000) {
            error_msg("Invalid paragraph width: %s", arg);
            return;
        }
    }
    format_paragraph(width);
}

static void cmd_wresize(const CommandArgs *a)
{
    if (window->frame->parent == NULL) {
        // Only window
        return;
    }

    const char *pf = a->flags;
    ResizeDirection dir = RESIZE_DIRECTION_AUTO;
    while (*pf) {
        switch (*pf) {
        case 'h':
            dir = RESIZE_DIRECTION_HORIZONTAL;
            break;
        case 'v':
            dir = RESIZE_DIRECTION_VERTICAL;
            break;
        }
        pf++;
    }

    const char *arg = a->args[0];
    if (arg) {
        int n;
        if (!str_to_int(arg, &n)) {
            error_msg("Invalid resize value: %s", arg);
            return;
        }
        if (arg[0] == '+' || arg[0] == '-') {
            add_to_frame_size(window->frame, dir, n);
        } else {
            resize_frame(window->frame, dir, n);
        }
    } else {
        equalize_frame_sizes(window->frame->parent);
    }
    mark_everything_changed();
    debug_frames();
}

static void cmd_wsplit(const CommandArgs *a)
{
    const char *pf = a->flags;
    bool before = false;
    bool vertical = false;
    bool root = false;

    while (*pf) {
        switch (*pf) {
        case 'b':
            // Add new window before current window
            before = true;
            break;
        case 'h':
            // Split horizontally to get vertical layout
            vertical = true;
            break;
        case 'r':
            // Split root frame instead of current window
            root = true;
            break;
        }
        pf++;
    }

    Frame *f;
    if (root) {
        f = split_root(vertical, before);
    } else {
        f = split_frame(window, vertical, before);
    }

    View *save = view;
    window = f->window;
    view = NULL;
    buffer = NULL;
    mark_everything_changed();

    if (a->args[0]) {
        window_open_files(window, a->args, NULL);
    } else {
        View *new = window_add_buffer(window, save->buffer);
        new->cursor = save->cursor;
        set_view(new);
    }

    if (window->views.count == 0) {
        // Open failed, remove new window
        remove_frame(window->frame);

        view = save;
        buffer = view->buffer;
        window = view->window;
    }

    debug_frames();
}

static void cmd_wswap(const CommandArgs* UNUSED_ARG(a))
{
    Frame *parent = window->frame->parent;
    if (parent == NULL) {
        return;
    }

    size_t i = ptr_array_idx(&parent->frames, window->frame);
    size_t j = i + 1;
    if (j == parent->frames.count) {
        j = 0;
    }

    Frame *tmp = parent->frames.ptrs[i];
    parent->frames.ptrs[i] = parent->frames.ptrs[j];
    parent->frames.ptrs[j] = tmp;
    mark_everything_changed();
}

// Prevent Clang whining about .max_args = -1
#if HAS_WARNING("-Wbitfield-constant-conversion")
 IGNORE_WARNING("-Wbitfield-constant-conversion")
#endif

const Command commands[] = {
    {"alias", "", 2, 2, cmd_alias},
    {"bind", "", 1, 2, cmd_bind},
    {"bof", "", 0, 0, cmd_bof},
    {"bol", "cs", 0, 0, cmd_bol},
    {"bolsf", "", 0, 0, cmd_bolsf},
    {"case", "lu", 0, 0, cmd_case},
    {"cd", "", 1, 1, cmd_cd},
    {"center-view", "", 0, 0, cmd_center_view},
    {"clear", "", 0, 0, cmd_clear},
    {"close", "fqw", 0, 0, cmd_close},
    {"command", "", 0, 1, cmd_command},
    {"compile", "-1ps", 2, -1, cmd_compile},
    {"copy", "k", 0, 0, cmd_copy},
    {"cut", "", 0, 0, cmd_cut},
    {"delete", "", 0, 0, cmd_delete},
    {"delete-eol", "n", 0, 0, cmd_delete_eol},
    {"delete-word", "s", 0, 0, cmd_delete_word},
    {"down", "cl", 0, 0, cmd_down},
    {"eof", "", 0, 0, cmd_eof},
    {"eol", "c", 0, 0, cmd_eol},
    {"eolsf", "", 0, 0, cmd_eolsf},
    {"erase", "", 0, 0, cmd_erase},
    {"erase-bol", "", 0, 0, cmd_erase_bol},
    {"erase-word", "s", 0, 0, cmd_erase_word},
    {"errorfmt", "i", 2, 18, cmd_errorfmt},
    {"eval", "-", 1, -1, cmd_eval},
    {"filter", "-", 1, -1, cmd_filter},
    {"ft", "-bcfi", 2, -1, cmd_ft},
    {"git-open", "", 0, 0, cmd_git_open},
    {"hi", "-", 0, -1, cmd_hi},
    {"include", "b", 1, 1, cmd_include},
    {"insert", "km", 1, 1, cmd_insert},
    {"insert-builtin", "", 1, 1, cmd_insert_builtin},
    {"join", "", 0, 0, cmd_join},
    {"left", "c", 0, 0, cmd_left},
    {"line", "", 1, 1, cmd_line},
    {"load-syntax", "", 1, 1, cmd_load_syntax},
    {"move-tab", "", 1, 1, cmd_move_tab},
    {"msg", "np", 0, 0, cmd_msg},
    {"new-line", "", 0, 0, cmd_new_line},
    {"next", "", 0, 0, cmd_next},
    {"open", "e=g", 0, -1, cmd_open},
    {"option", "-r", 3, -1, cmd_option},
    {"paste", "c", 0, 0, cmd_paste},
    {"pgdown", "cl", 0, 0, cmd_pgdown},
    {"pgup", "cl", 0, 0, cmd_pgup},
    {"pipe-from", "-ms", 1, -1, cmd_pipe_from},
    {"pipe-to", "-", 1, -1, cmd_pipe_to},
    {"prev", "", 0, 0, cmd_prev},
    {"quit", "fp", 0, 0, cmd_quit},
    {"redo", "", 0, 1, cmd_redo},
    {"refresh", "", 0, 0, cmd_refresh},
    {"repeat", "", 2, -1, cmd_repeat},
    {"replace", "bcgi", 2, 2, cmd_replace},
    {"right", "c", 0, 0, cmd_right},
    {"run", "-ps", 1, -1, cmd_run},
    {"save", "de=fpu", 0, 1, cmd_save},
    {"scroll-down", "", 0, 0, cmd_scroll_down},
    {"scroll-pgdown", "", 0, 0, cmd_scroll_pgdown},
    {"scroll-pgup", "", 0, 0, cmd_scroll_pgup},
    {"scroll-up", "", 0, 0, cmd_scroll_up},
    {"search", "Hnprw", 0, 1, cmd_search},
    {"select", "bkl", 0, 0, cmd_select},
    {"set", "gl", 1, -1, cmd_set},
    {"setenv", "", 2, 2, cmd_setenv},
    {"shift", "", 1, 1, cmd_shift},
    {"show", "c", 1, 2, cmd_show},
    {"suspend", "", 0, 0, cmd_suspend},
    {"tag", "r", 0, 1, cmd_tag},
    {"toggle", "glv", 1, -1, cmd_toggle},
    {"undo", "", 0, 0, cmd_undo},
    {"unselect", "", 0, 0, cmd_unselect},
    {"up", "cl", 0, 0, cmd_up},
    {"view", "", 1, 1, cmd_view},
    {"wclose", "f", 0, 0, cmd_wclose},
    {"wflip", "", 0, 0, cmd_wflip},
    {"wnext", "", 0, 0, cmd_wnext},
    {"word-bwd", "cs", 0, 0, cmd_word_bwd},
    {"word-fwd", "cs", 0, 0, cmd_word_fwd},
    {"wprev", "", 0, 0, cmd_wprev},
    {"wrap-paragraph", "", 0, 1, cmd_wrap_paragraph},
    {"wresize", "hv", 0, 1, cmd_wresize},
    {"wsplit", "bhr", 0, -1, cmd_wsplit},
    {"wswap", "", 0, 0, cmd_wswap},
    {"", "", 0, 0, NULL}
};

UNIGNORE_WARNINGS
