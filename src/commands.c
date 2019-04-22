#include <glob.h>
#include <unistd.h>
#include "alias.h"
#include "bind.h"
#include "change.h"
#include "cmdline.h"
#include "command.h"
#include "common.h"
#include "config.h"
#include "debug.h"
#include "edit.h"
#include "editor.h"
#include "encoding/convert.h"
#include "encoding/encoding.h"
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
#include "util/path.h"
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

static void cmd_alias(const char* UNUSED_ARG(pf), char **args)
{
    add_alias(args[0], args[1]);
}

static void cmd_bind(const char* UNUSED_ARG(pf), char **args)
{
    if (args[1]) {
        add_binding(args[0], args[1]);
    } else {
        remove_binding(args[0]);
    }
}

static void cmd_bof(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    move_bof();
}

static void cmd_bol(const char *pf, char** UNUSED_ARG(args))
{
    handle_select_chars_flag(pf);
    if (strchr(pf, 's')) {
        move_bol_smart();
    } else {
        move_bol();
    }
}

static void cmd_bolsf(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
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

static void cmd_case(const char *pf, char** UNUSED_ARG(args))
{
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

static void cmd_cd(const char* UNUSED_ARG(pf), char **args)
{
    char *dir = args[0];
    char cwd[8192];
    char *cwdp = NULL;
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

static void cmd_center_view(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    view->force_center = true;
}

static void cmd_clear(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    clear_lines();
}

static void cmd_close(const char *pf, char** UNUSED_ARG(args))
{
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

static void cmd_command(const char* UNUSED_ARG(pf), char **args)
{
    set_input_mode(INPUT_COMMAND);
    if (args[0]) {
        cmdline_set_text(&editor.cmdline, args[0]);
    }
}

static void cmd_compile(const char *pf, char **args)
{
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

    const char *name = *args++;
    Compiler *c = find_compiler(name);
    if (!c) {
        error_msg("No such error parser %s", name);
        return;
    }
    clear_messages();
    spawn_compiler(args, flags, c);
    if (message_count()) {
        activate_current_message_save();
    }
}

static void cmd_copy(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    BlockIter save = view->cursor;

    if (view->selection) {
        copy(prepare_selection(view), view->selection == SELECT_LINES);
        unselect();
    } else {
        block_iter_bol(&view->cursor);
        BlockIter tmp = view->cursor;
        copy(block_iter_eat_line(&tmp), true);
    }
    view->cursor = save;
}

static void cmd_cut(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    int x = view_get_preferred_x(view);

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

static void cmd_delete(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    delete_ch();
}

static void cmd_delete_eol(const char *pf, char** UNUSED_ARG(args))
{
    BlockIter bi = view->cursor;

    if (view->selection) {
        return;
    }

    if (*pf) {
        CodePoint ch;
        buffer_get_char(&view->cursor, &ch);
        if (ch == '\n') {
            delete_ch();
            return;
        }
    }

    buffer_delete_bytes(block_iter_eol(&bi));
}

static void cmd_delete_word(const char *pf, char** UNUSED_ARG(args))
{
    bool skip_non_word = *pf == 's';
    BlockIter bi = view->cursor;

    buffer_delete_bytes(word_fwd(&bi, skip_non_word));
}

static void cmd_down(const char *pf, char** UNUSED_ARG(args))
{
    handle_select_chars_or_lines_flags(pf);
    move_down(1);
}

static void cmd_eof(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    move_eof();
}

static void cmd_eol(const char *pf, char** UNUSED_ARG(args))
{
    handle_select_chars_flag(pf);
    move_eol();
}

static void cmd_eolsf(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
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

static void cmd_erase(const char* UNUSED_ARG(pf), char**  UNUSED_ARG(args))
{
    erase();
}

static void cmd_erase_bol(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    buffer_erase_bytes(block_iter_bol(&view->cursor));
}

static void cmd_erase_word(const char *pf, char** UNUSED_ARG(args))
{
    bool skip_non_word = *pf == 's';
    buffer_erase_bytes(word_bwd(&view->cursor, skip_non_word));
}

static void cmd_errorfmt(const char *pf, char **args)
{
    bool ignore = false;

    while (*pf) {
        switch (*pf) {
        case 'i':
            ignore = true;
            break;
        }
        pf++;
    }
    add_error_fmt(args[0], ignore, args[1], args + 2);
}

static void cmd_eval(const char* UNUSED_ARG(pf), char **args)
{
    FilterData data = FILTER_DATA_INIT;
    if (spawn_filter(args, &data)) {
        return;
    }
    exec_config(commands, data.out, data.out_len);
    free(data.out);
}

static void cmd_filter(const char* UNUSED_ARG(pf), char **args)
{
    FilterData data;
    BlockIter save = view->cursor;

    if (view->selection) {
        data.in_len = prepare_selection(view);
    } else {
        Block *blk;
        data.in_len = 0;
        list_for_each_entry(blk, &buffer->blocks, node) {
            data.in_len += blk->size;
        }
        move_bof();
    }

    data.in = block_iter_get_bytes(&view->cursor, data.in_len);
    if (spawn_filter(args, &data)) {
        free(data.in);
        view->cursor = save;
        return;
    }

    free(data.in);
    buffer_replace_bytes(data.in_len, data.out, data.out_len);
    free(data.out);

    unselect();
}

static void cmd_ft(const char *pf, char **args)
{
    enum detect_type dt = FT_EXTENSION;

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

    if (args[0][0] == '\0') {
        error_msg("Filetype can't be blank");
        return;
    }

    for (size_t i = 1; args[i]; i++) {
        add_filetype(args[0], args[i], dt);
    }
}

static void cmd_git_open(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    set_input_mode(INPUT_GIT_OPEN);
    git_open_reload();
}

static void cmd_hi(const char* UNUSED_ARG(pf), char **args)
{
    TermColor color;

    if (args[0] == NULL) {
        exec_reset_colors_rc();
        remove_extra_colors();
    } else if (parse_term_color(&color, args + 1)) {
        color.fg = convert_color_to_nearest_supported(color.fg);
        color.bg = convert_color_to_nearest_supported(color.bg);
        set_highlight_color(args[0], &color);
    }

    // Don't call update_all_syntax_colors() needlessly.
    // It is called right after config has been loaded.
    if (editor.status != EDITOR_INITIALIZING) {
        update_all_syntax_colors();
        mark_everything_changed();
    }
}

static void cmd_include(const char *pf, char **args)
{
    ConfigFlags flags = CFG_MUST_EXIST;
    if (*pf == 'b') {
        flags |= CFG_BUILTIN;
    }
    read_config(commands, args[0], flags);
}

static void cmd_insert(const char *pf, char **args)
{
    const char *str = args[0];

    if (strchr(pf, 'k')) {
        for (size_t i = 0; str[i]; i++) {
            insert_ch(str[i]);
        }
    } else {
        size_t del_len = 0;
        size_t ins_len = strlen(str);

        if (view->selection) {
            del_len = prepare_selection(view);
            unselect();
        }

        buffer_replace_bytes(del_len, str, ins_len);
        if (strchr(pf, 'm')) {
            block_iter_skip_bytes(&view->cursor, ins_len);
        }
    }
}

static void cmd_insert_builtin(const char* UNUSED_ARG(pf), char **args)
{
    const char *name = args[0];
    const BuiltinConfig *cfg = get_builtin_config(name);
    if (cfg) {
        buffer_insert_bytes(cfg->text.data, cfg->text.length);
    } else {
        error_msg("No built-in config with name '%s'", name);
    }
}

static void cmd_join(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    join_lines();
}

static void cmd_left(const char *pf, char** UNUSED_ARG(args))
{
    handle_select_chars_flag(pf);
    move_cursor_left();
}

static void cmd_line(const char* UNUSED_ARG(pf), char **args)
{
    int x = view_get_preferred_x(view);
    size_t line;
    if (!str_to_size(args[0], &line) || line == 0) {
        error_msg("Invalid line number: %s", args[0]);
        return;
    }
    move_to_line(view, line);
    move_to_preferred_x(x);
}

static void cmd_load_syntax(const char* UNUSED_ARG(pf), char **args)
{
    const char *filename = args[0];
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

static void cmd_move_tab(const char* UNUSED_ARG(pf), char **args)
{
    size_t j, i = ptr_array_idx(&window->views, view);
    char *str = args[0];

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

static void cmd_msg(const char *pf, char** UNUSED_ARG(args))
{
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

static void cmd_new_line(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    new_line();
}

static void cmd_next(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    set_view(ptr_array_next(&window->views, view));
}

static void cmd_open(const char *pf, char **args)
{
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

    bool dealloc_encoding = true;
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
            free_encoding(&buffer->encoding);
            buffer->encoding = encoding;
            // buffer now owns encoding -- don't free it below
            dealloc_encoding = false;
        }
    } else if (!paths[1]) {
        // Previous view is remembered when opening single file
        window_open_file(window, paths[0], &encoding);
    } else {
        // It makes no sense to remember previous view when opening
        // multiple files
        window_open_files(window, paths, &encoding);
    }

    if (dealloc_encoding) {
        free_encoding(&encoding);
    }
    if (use_glob) {
        globfree(&globbuf);
    }
}

static void cmd_option(const char *pf, char **args)
{
    size_t argc = count_strings(args);
    BUG_ON(argc == 0);
    char **strs = args + 1;
    size_t count = argc - 1;

    if (argc % 2 == 0) {
        error_msg("Missing option value");
        return;
    }
    if (!validate_local_options(strs)) {
        return;
    }

    if (*pf) {
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

static void cmd_pass_through(const char *pf, char **args)
{
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
    if (spawn_filter(args, &data)) {
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

static void cmd_paste(const char *pf, char** UNUSED_ARG(args))
{
    if (pf[0]) {
        paste(true);
    } else {
        paste(false);
    }
}

static void cmd_pgdown(const char *pf, char** UNUSED_ARG(args))
{
    handle_select_chars_or_lines_flags(pf);

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

static void cmd_pgup(const char *pf, char** UNUSED_ARG(args))
{
    handle_select_chars_or_lines_flags(pf);

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

static void cmd_prev(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    set_view(ptr_array_prev(&window->views, view));
}

static void cmd_quit(const char *pf, char** UNUSED_ARG(args))
{
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

static void cmd_redo(const char* UNUSED_ARG(pf), char **args)
{
    unsigned long change_id = 0;
    if (args[0]) {
        if (!str_to_ulong(args[0], &change_id) || change_id == 0) {
            error_msg("Invalid change id: %s", args[0]);
            return;
        }
    }
    if (redo(change_id)) {
        unselect();
    }
}

static void cmd_refresh(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    mark_everything_changed();
}

static void cmd_repeat(const char *pf, char **args)
{
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

    args += 2;
    pf = parse_args(args, cmd->flags, cmd->min_args, cmd->max_args);
    if (pf) {
        while (count-- > 0) {
            cmd->cmd(pf, args);
        }
    }
}

static void cmd_replace(const char *pf, char **args)
{
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
    reg_replace(args[0], args[1], flags);
}

static void cmd_right(const char *pf, char** UNUSED_ARG(args))
{
    handle_select_chars_flag(pf);
    move_cursor_right();
}

static void cmd_run(const char *pf, char **args)
{
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
    spawn(args, fd, prompt);
}

static bool stat_changed(const struct stat *const a, const struct stat *const b)
{
    // Don't compare st_mode because we allow chmod 755 etc.
    return a->st_mtime != b->st_mtime ||
        a->st_dev != b->st_dev ||
        a->st_ino != b->st_ino;
}

static void cmd_save(const char *pf, char **args)
{
    char *absolute = buffer->abs_filename;
    Encoding encoding = buffer->encoding;
    bool encoding_alloced = false;
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
        encoding_alloced = true;
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
    if (encoding_alloced) {
        free_encoding(&buffer->encoding);
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
    if (encoding_alloced) {
        free_encoding(&encoding);
    }
}

static void cmd_scroll_down(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    view->vy++;
    if (view->cy < view->vy) {
        move_down(1);
    }
}

static void cmd_scroll_pgdown(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    int max = buffer->nl - window->edit_h + 1;

    if (view->vy < max && max > 0) {
        int count = window->edit_h - 1;

        if (view->vy + count > max) {
            count = max - view->vy;
        }
        view->vy += count;
        move_down(count);
    } else if (view->cy < buffer->nl) {
        move_down(buffer->nl - view->cy);
    }
}

static void cmd_scroll_pgup(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    if (view->vy > 0) {
        int count = window->edit_h - 1;

        if (count > view->vy) {
            count = view->vy;
        }
        view->vy -= count;
        move_up(count);
    } else if (view->cy > 0) {
        move_up(view->cy);
    }
}

static void cmd_scroll_up(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    if (view->vy) {
        view->vy--;
    }
    if (view->vy + window->edit_h <= view->cy) {
        move_up(1);
    }
}

static void cmd_search(const char *pf, char **args)
{
    bool history = true;
    char cmd = 0;
    bool w = false;
    SearchDirection dir = SEARCH_FWD;
    char *pattern = args[0];

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
        pattern = xnew(char, len);
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

        if (pattern != args[0]) {
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

static void cmd_select(const char *pf, char** UNUSED_ARG(args))
{
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

static void cmd_set(const char *pf, char **args)
{
    bool global = false;
    bool local = false;
    int count = count_strings(args);

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

    if (count == 1) {
        set_bool_option(args[0], local, global);
        return;
    }
    if (count % 2) {
        error_msg("One or even number of arguments expected.");
        return;
    }
    for (size_t i = 0; args[i]; i += 2) {
        set_option(args[i], args[i + 1], local, global);
    }
}

static void cmd_setenv(const char* UNUSED_ARG(pf), char **args)
{
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

static void cmd_shift(const char* UNUSED_ARG(pf), char **args)
{
    int count;
    if (!str_to_int(args[0], &count)) {
        error_msg("Invalid number: %s", args[0]);
        return;
    }
    if (count == 0) {
        error_msg("Count must be non-zero.");
        return;
    }
    shift_lines(count);
}

static void cmd_show_bindings(const char* UNUSED_ARG(p), char** UNUSED_ARG(a))
{
    char tmp[32] = "/tmp/.dte.binds.XXXXXX";
    int fd = mkstemp(tmp);
    if (fd < 0) {
        error_msg("mkstemp() failed: %s", strerror(errno));
        return;
    }

    String s = dump_bindings();
    xwrite(fd, s.buffer, s.len);
    close(fd);
    string_free(&s);

    PointerArray a = PTR_ARRAY_INIT;
    ptr_array_add(&a, xstrdup("run"));
    ptr_array_add(&a, xstrdup(editor.pager));
    ptr_array_add(&a, xstrdup(tmp));
    ptr_array_add(&a, NULL);
    run_commands(commands, &a);
    ptr_array_free(&a);

    unlink(tmp);
}

static void cmd_suspend(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    suspend();
}

static void cmd_tag(const char *pf, char **args)
{
    bool pop = false;

    while (*pf) {
        switch (*pf) {
        case 'r':
            pop = true;
            break;
        }
        pf++;
    }

    if (pop) {
        pop_file_location();
        return;
    }

    clear_messages();
    TagFile *tf = load_tag_file();
    if (tf == NULL) {
        error_msg("No tag file.");
        return;
    }

    const char *name = args[0];
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

static void cmd_toggle(const char *pf, char **args)
{
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

    if (args[1]) {
        toggle_option_values(args[0], global, verbose, args + 1);
    } else {
        toggle_option(args[0], global, verbose);
    }
}

static void cmd_undo(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    if (undo()) {
        unselect();
    }
}

static void cmd_unselect(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    unselect();
}

static void cmd_up(const char *pf, char** UNUSED_ARG(args))
{
    handle_select_chars_or_lines_flags(pf);
    move_up(1);
}

static void cmd_view(const char* UNUSED_ARG(pf), char **args)
{
    BUG_ON(window->views.count == 0);
    size_t idx;
    if (streq(args[0], "last")) {
        idx = window->views.count - 1;
    } else {
        if (!str_to_size(args[0], &idx) || idx == 0) {
            error_msg("Invalid view index: %s", args[0]);
            return;
        }
        idx--;
        if (idx > window->views.count - 1) {
            idx = window->views.count - 1;
        }
    }
    set_view(window->views.ptrs[idx]);
}

static void cmd_wclose(const char *pf, char** UNUSED_ARG(args))
{
    View *v = window_find_unclosable_view(window, view_can_close);
    bool force = !!*pf;

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

static void cmd_wflip(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    Frame *f = window->frame;

    if (f->parent == NULL) {
        return;
    }

    f->parent->vertical ^= 1;
    mark_everything_changed();
}

static void cmd_wnext(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    window = next_window(window);
    set_view(window->view);
    mark_everything_changed();
    debug_frames();
}

static void cmd_word_bwd(const char *pf, char** UNUSED_ARG(args))
{
    handle_select_chars_flag(pf);
    bool skip_non_word = strchr(pf, 's');
    word_bwd(&view->cursor, skip_non_word);
    view_reset_preferred_x(view);
}

static void cmd_word_fwd(const char *pf, char** UNUSED_ARG(args))
{
    handle_select_chars_flag(pf);
    bool skip_non_word = strchr(pf, 's');
    word_fwd(&view->cursor, skip_non_word);
    view_reset_preferred_x(view);
}

static void cmd_wprev(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
{
    window = prev_window(window);
    set_view(window->view);
    mark_everything_changed();
    debug_frames();
}

static void cmd_wrap_paragraph(const char * UNUSED_ARG(pf), char **args)
{
    size_t width = (size_t)buffer->options.text_width;
    if (args[0]) {
        if (!str_to_size(args[0], &width) || width == 0 || width > 1000) {
            error_msg("Invalid paragraph width: %s", args[0]);
            return;
        }
    }
    format_paragraph(width);
}

static void cmd_wresize(const char *pf, char **args)
{
    ResizeDirection dir = RESIZE_DIRECTION_AUTO;
    const char *arg = *args;

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
    if (window->frame->parent == NULL) {
        // Only window
        return;
    }
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

static void cmd_wsplit(const char *pf, char **args)
{
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

    if (*args) {
        window_open_files(window, args, NULL);
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

static void cmd_wswap(const char* UNUSED_ARG(pf), char** UNUSED_ARG(args))
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
    {"copy", "", 0, 0, cmd_copy},
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
    {"errorfmt", "i", 2, 6, cmd_errorfmt},
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
    {"pass-through", "-ms", 1, -1, cmd_pass_through},
    {"paste", "c", 0, 0, cmd_paste},
    {"pgdown", "cl", 0, 0, cmd_pgdown},
    {"pgup", "cl", 0, 0, cmd_pgup},
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
    {"show-bindings", "", 0, 0, cmd_show_bindings},
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
