#include <errno.h>
#include <glob.h>
#include <sys/stat.h>
#include <unistd.h>
#include "alias.h"
#include "bind.h"
#include "change.h"
#include "cmdline.h"
#include "command.h"
#include "completion.h"
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
#include "terminal/terminal.h"
#include "util/line-iter.h"
#include "util/path.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
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

static char last_flag_or_default(const CommandArgs *a, char def)
{
    size_t n = a->nr_flags;
    return n ? a->flags[n - 1] : def;
}

static char last_flag(const CommandArgs *a)
{
    return last_flag_or_default(a, 0);
}

static bool has_flag(const CommandArgs *a, int flag)
{
    size_t n = a->nr_flags;
    return n && !!memchr(a->flags, flag, n);
}

static void handle_select_chars_flag(const CommandArgs *a)
{
    do_selection(has_flag(a, 'c') ? SELECT_CHARS : SELECT_NONE);
}

static void handle_select_chars_or_lines_flags(const CommandArgs *a)
{
    SelectionType sel = SELECT_NONE;
    if (has_flag(a, 'l')) {
        sel = SELECT_LINES;
    } else if (has_flag(a, 'c')) {
        sel = SELECT_CHARS;
    }
    do_selection(sel);
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
    do_selection(SELECT_NONE);
    move_bof();
}

static void cmd_bol(const CommandArgs *a)
{
    handle_select_chars_flag(a);
    if (has_flag(a, 's')) {
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
    int mode = last_flag_or_default(a, 't');
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
        perror_msg("cd");
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

    for (size_t i = 0, n = editor.buffers.count; i < n; i++) {
        Buffer *b = editor.buffers.ptrs[i];
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
    bool force = false;
    bool prompt = false;
    bool allow_quit = false;
    bool allow_wclose = false;
    for (const char *pf = a->flags; *pf; pf++) {
        switch (*pf) {
        case 'f':
            force = true;
            break;
        case 'p':
            prompt = true;
            break;
        case 'q':
            allow_quit = true;
            break;
        case 'w':
            allow_wclose = true;
            break;
        }
    }

    if (!view_can_close(view) && !force) {
        if (prompt) {
            if (dialog_prompt("Close without saving changes? [y/N]", "ny") != 'y') {
                return;
            }
        } else {
            error_msg (
                "The buffer is modified. "
                "Save or run 'close -f' to close without saving."
            );
            return;
        }
    }

    if (allow_quit && editor.buffers.count == 1 && root_frame->frames.count <= 1) {
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
    SpawnFlags flags = SPAWN_DEFAULT;
    for (const char *pf = a->flags; *pf; pf++) {
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
        do_selection(SELECT_NONE);
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
    handle_select_chars_or_lines_flags(a);
    move_down(1);
}

static void cmd_eof(const CommandArgs* UNUSED_ARG(a))
{
    do_selection(SELECT_NONE);
    move_eof();
}

static void cmd_eol(const CommandArgs *a)
{
    handle_select_chars_flag(a);
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
    SpawnContext ctx = {
        .argv = a->args,
        .input = STRING_VIEW_INIT,
        .output = STRING_INIT,
        .flags = SPAWN_DEFAULT
    };
    if (!spawn_filter(&ctx)) {
        return;
    }
    exec_config(&commands, ctx.output.buffer, ctx.output.len);
    string_free(&ctx.output);
}

static void cmd_exec_open(const CommandArgs *a)
{
    SpawnContext ctx = {
        .argv = a->args,
        .output = STRING_INIT,
        .flags = has_flag(a, 's') ? SPAWN_QUIET : SPAWN_DEFAULT
    };
    if (!spawn_source(&ctx)) {
        return;
    }

    PointerArray filenames = PTR_ARRAY_INIT;
    for (size_t pos = 0, size = ctx.output.len; pos < size; ) {
        char *filename = buf_next_line(ctx.output.buffer, &pos, size);
        if (filename[0] != '\0') {
            ptr_array_append(&filenames, filename);
        }
    }

    ptr_array_append(&filenames, NULL);
    window_open_files(window, (char**)filenames.ptrs, NULL);
    ptr_array_free_array(&filenames);
    string_free(&ctx.output);
}

static void cmd_filter(const CommandArgs *a)
{
    BlockIter save = view->cursor;
    SpawnContext ctx = {
        .argv = a->args,
        .input = STRING_VIEW_INIT,
        .output = STRING_INIT,
        .flags = SPAWN_DEFAULT
    };

    if (view->selection) {
        ctx.input.length = prepare_selection(view);
    } else if (has_flag(a, 'l')) {
        StringView line;
        move_bol();
        fill_line_ref(&view->cursor, &line);
        ctx.input.length = line.length;
    } else {
        Block *blk;
        block_for_each(blk, &buffer->blocks) {
            ctx.input.length += blk->size;
        }
        move_bof();
    }

    char *input = block_iter_get_bytes(&view->cursor, ctx.input.length);
    ctx.input.data = input;
    if (!spawn_filter(&ctx)) {
        free(input);
        view->cursor = save;
        return;
    }

    free(input);
    buffer_replace_bytes(ctx.input.length, ctx.output.buffer, ctx.output.len);
    string_free(&ctx.output);
    unselect();
}

static void cmd_ft(const CommandArgs *a)
{
    FileDetectionType dt = FT_EXTENSION;
    if (a->nr_flags) {
        switch (a->flags[a->nr_flags - 1]) {
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

static void cmd_hi(const CommandArgs *a)
{
    char **args = a->args;
    TermColor color;
    if (args[0] == NULL) {
        exec_builtin_color_reset();
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
    for (const char *pf = a->flags; *pf; pf++) {
        switch (*pf) {
        case 'b':
            flags |= CFG_BUILTIN;
            break;
        case 'q':
            flags &= ~CFG_MUST_EXIST;
            break;
        }
    }
    read_config(&commands, a->args[0], flags);
}

static void cmd_insert(const CommandArgs *a)
{
    const char *str = a->args[0];
    if (has_flag(a, 'k')) {
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

    if (has_flag(a, 'm')) {
        block_iter_skip_bytes(&view->cursor, ins_len);
    }
}

static void cmd_join(const CommandArgs* UNUSED_ARG(a))
{
    join_lines();
}

static void cmd_left(const CommandArgs *a)
{
    handle_select_chars_flag(a);
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

    do_selection(SELECT_NONE);
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
            load_syntax_file(filename, CFG_MUST_EXIST, &err);
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
    char dir = last_flag(a);
    do_selection(SELECT_NONE);
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
    size_t i = ptr_array_idx(&window->views, view);
    size_t n = window->views.count;
    BUG_ON(i >= n);
    set_view(window->views.ptrs[(i + 1) % n]);
}

static bool xglob(char **args, glob_t *globbuf)
{
    BUG_ON(!args);
    BUG_ON(!*args);
    int err = glob(*args, GLOB_NOCHECK, NULL, globbuf);
    while (err == 0 && *++args) {
        err = glob(*args, GLOB_NOCHECK | GLOB_APPEND, NULL, globbuf);
    }
    if (likely(err == 0 && globbuf->gl_pathc > 0)) {
        return true;
    }
    error_msg("glob: %s", (err == GLOB_NOSPACE) ? strerror(ENOMEM) : "failed");
    globfree(globbuf);
    return false;
}

static void cmd_open(const CommandArgs *a)
{
    char **args = a->args;
    const char *requested_encoding = NULL;
    bool use_glob = false;
    bool temporary = false;
    for (const char *pf = a->flags; *pf; pf++) {
        switch (*pf) {
        case 'e':
            requested_encoding = *args++;
            break;
        case 'g':
            use_glob = args[0] ? true : false;
            break;
        case 't':
            temporary = true;
            break;
        }
    }

    if (temporary && a->nr_args > 0) {
        error_msg("'open -t' can't be used with filename arguments");
        return;
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
            error_msg("Unsupported encoding: '%s'", requested_encoding);
            return;
        }
        encoding = encoding_from_name(requested_encoding);
    }

    char **paths = args;
    glob_t globbuf;
    if (use_glob) {
        if (!xglob(args, &globbuf)) {
            return;
        }
        paths = globbuf.gl_pathv;
    }

    if (!paths[0]) {
        window_open_new_file(window);
        buffer->temporary = temporary;
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

static void cmd_blkdown(const CommandArgs *a)
{
    handle_select_chars_or_lines_flags(a);

    // If current line is blank, skip past consecutive blank lines
    StringView line;
    fetch_this_line(&view->cursor, &line);
    if (strview_isblank(&line)) {
        while (block_iter_next_line(&view->cursor)) {
            fill_line_ref(&view->cursor, &line);
            if (!strview_isblank(&line)) {
                break;
            }
        }
    }

    // Skip past non-blank lines
    while (block_iter_next_line(&view->cursor)) {
        fill_line_ref(&view->cursor, &line);
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
}

static void cmd_blkup(const CommandArgs *a)
{
    handle_select_chars_or_lines_flags(a);

    // If cursor is on the first line, just move to bol
    if (view->cy == 0) {
        block_iter_bol(&view->cursor);
        return;
    }

    // If current line is blank, skip past consecutive blank lines
    StringView line;
    fetch_this_line(&view->cursor, &line);
    if (strview_isblank(&line)) {
        while (block_iter_prev_line(&view->cursor)) {
            fill_line_ref(&view->cursor, &line);
            if (!strview_isblank(&line)) {
                break;
            }
        }
    }

    // Skip past non-blank lines
    while (block_iter_prev_line(&view->cursor)) {
        fill_line_ref(&view->cursor, &line);
        if (strview_isblank(&line)) {
            break;
        }
    }
}

static void cmd_paste(const CommandArgs *a)
{
    bool at_cursor = a->flags[0] == 'c';
    paste(at_cursor);
}

static void cmd_pgdown(const CommandArgs *a)
{
    handle_select_chars_or_lines_flags(a);

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
    handle_select_chars_or_lines_flags(a);

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
    bool strip_nl = false;
    bool move = false;
    for (const char *pf = a->flags; *pf; pf++) {
        switch (*pf) {
        case 'm':
            move = true;
            break;
        case 's':
            strip_nl = true;
            break;
        }
    }

    SpawnContext ctx = {
        .argv = a->args,
        .output = STRING_INIT,
        .flags = SPAWN_QUIET
    };
    if (!spawn_source(&ctx)) {
        return;
    }

    size_t del_len = 0;
    if (view->selection) {
        del_len = prepare_selection(view);
        unselect();
    }

    size_t ins_len = ctx.output.len;
    if (strip_nl) {
        if (ins_len && ctx.output.buffer[ins_len - 1] == '\n') {
            ins_len--;
            if (ins_len && ctx.output.buffer[ins_len - 1] == '\r') {
                ins_len--;
            }
        }
    }

    buffer_replace_bytes(del_len, ctx.output.buffer, ins_len);
    string_free(&ctx.output);

    if (move) {
        block_iter_skip_bytes(&view->cursor, ins_len);
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
    } else if (has_flag(a, 'l')) {
        StringView line;
        move_bol();
        fill_line_ref(&view->cursor, &line);
        input_len = line.length;
    } else {
        Block *blk;
        block_for_each(blk, &buffer->blocks) {
            input_len += blk->size;
        }
        move_bof();
    }

    char *input = block_iter_get_bytes(&view->cursor, input_len);
    SpawnContext ctx = {
        .argv = a->args,
        .input = string_view(input, input_len),
        .flags = SPAWN_DEFAULT
    };
    spawn_sink(&ctx);
    free(input);

    // Restore cursor and selection offsets, instead of calling unselect()
    view->cursor = saved_cursor;
    view->sel_so = saved_sel_so;
    view->sel_eo = saved_sel_eo;
}

static void cmd_prev(const CommandArgs* UNUSED_ARG(a))
{
    size_t i = ptr_array_idx(&window->views, view);
    size_t n = window->views.count;
    BUG_ON(i >= n);
    set_view(window->views.ptrs[(i ? i : n) - 1]);
}

static void cmd_quit(const CommandArgs *a)
{
    if (has_flag(a, 'f')) {
        goto exit;
    }

    for (size_t i = 0, n = editor.buffers.count; i < n; i++) {
        Buffer *b = editor.buffers.ptrs[i];
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
            if (has_flag(a, 'p')) {
                if (dialog_prompt("Quit without saving changes? [y/N]", "ny") == 'y') {
                    goto exit;
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

exit:
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

    const Command *cmd = find_normal_command(args[1]);
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
    unsigned int flags = 0;
    for (const char *pf = a->flags; *pf; pf++) {
        switch (*pf) {
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
    handle_select_chars_flag(a);
    move_cursor_right();
}

static void cmd_run(const CommandArgs *a)
{
    SpawnContext ctx = {
        .argv = a->args,
        .flags = SPAWN_DEFAULT
    };
    for (const char *pf = a->flags; *pf; pf++) {
        switch (*pf) {
        case 'p':
            ctx.flags |= SPAWN_PROMPT;
            break;
        case 's':
            ctx.flags |= SPAWN_QUIET;
            break;
        }
    }
    spawn(&ctx);
}

static bool stat_changed(const Buffer *b, const struct stat *st)
{
    // Don't compare st_mode because we allow chmod 755 etc.
    return st->st_mtime != b->file.mtime
        || st->st_dev != b->file.dev
        || st->st_ino != b->file.ino;
}

static void cmd_save(const CommandArgs *a)
{
    if (buffer->stdout_buffer) {
        error_msg("special (stdout) buffer can't be saved");
        return;
    }

    char **args = a->args;
    char *absolute = buffer->abs_filename;
    Encoding encoding = buffer->encoding;
    const char *requested_encoding = NULL;
    bool force = false;
    bool prompt = false;
    bool crlf = buffer->crlf_newlines;
    mode_t old_mode = buffer->file.mode;
    struct stat st;
    bool new_locked = false;

    for (const char *pf = a->flags; *pf; pf++) {
        switch (*pf) {
        case 'd':
            crlf = true;
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
            crlf = false;
            break;
        }
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
                    if (!lock_file(absolute)) {
                        if (!force) {
                            error_msg("Can't lock file %s", absolute);
                            goto error;
                        }
                    } else {
                        buffer->locked = true;
                    }
                }
            } else {
                if (!lock_file(absolute)) {
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
            && stat_changed(buffer, &st)
        ) {
            error_msg (
                "File has been modified by another process."
                " Use 'save -f' to force overwrite."
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
                    if (!lock_file(absolute)) {
                        if (!force) {
                            error_msg("Can't lock file %s", absolute);
                            goto error;
                        }
                    } else {
                        buffer->locked = true;
                    }
                }
            } else {
                if (!lock_file(absolute)) {
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
        buffer->file.mode = st.st_mode;
    }
    if (save_buffer(buffer, absolute, &encoding, crlf)) {
        goto error;
    }

    buffer->saved_change = buffer->cur_change;
    buffer->readonly = false;
    buffer->temporary = false;
    buffer->crlf_newlines = crlf;
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
    char *pattern = a->args[0];
    bool history = true;
    char cmd = 0;
    bool w = false;
    SearchDirection dir = SEARCH_FWD;

    for (const char *pf = a->flags; *pf; pf++) {
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

    do_selection(SELECT_NONE);

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
    SelectionType sel = SELECT_CHARS;
    bool block = false;
    bool keep = false;

    for (const char *pf = a->flags; *pf; pf++) {
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
    bool global = has_flag(a, 'g');
    bool local = has_flag(a, 'l');
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
    const char *name = a->args[0];
    const char *value = a->args[1];
    if (streq(name, "DTE_VERSION")) {
        error_msg("$DTE_VERSION cannot be changed");
        return;
    }

    if (setenv(name, value, true) != 0) {
        switch (errno) {
        case EINVAL:
            error_msg("Invalid environment variable name '%s'", name);
            break;
        default:
            perror_msg("setenv");
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

static void show_alias(const char *alias_name, bool cflag)
{
    const char *cmd_str = find_alias(alias_name);
    if (cmd_str == NULL) {
        if (find_normal_command(alias_name)) {
            info_msg("%s is a built-in command, not an alias", alias_name);
        } else {
            info_msg("%s is not a known alias", alias_name);
        }
        return;
    }

    if (cflag) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, cmd_str);
    } else {
        info_msg("%s is aliased to: %s", alias_name, cmd_str);
    }
}

static void show_binding(const char *keystr, bool cflag)
{
    KeyCode key;
    if (!parse_key_string(&key, keystr)) {
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

    if (cflag) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, b->cmd_str);
    } else {
        info_msg("%s is bound to: %s", keystr, b->cmd_str);
    }
}

static void show_color(const char *color_name, bool cflag)
{
    const HlColor *hl = find_color(color_name);
    if (!hl) {
        error_msg("no color entry with name '%s'", color_name);
        return;
    }

    const char *color_str = term_color_to_string(&hl->color);
    if (cflag) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, color_str);
    } else {
        info_msg("color '%s' is set to: %s", color_name, color_str);
    }
}

static void show_include(const char *name, bool cflag)
{
    const BuiltinConfig *cfg = get_builtin_config(name);
    if (!cfg) {
        error_msg("no built-in config with name '%s'", name);
        return;
    }

    if (cflag) {
        buffer_insert_bytes(cfg->text.data, cfg->text.length);
        return;
    }

    View *v = window_open_new_file(window);
    v->buffer->temporary = true;
    do_insert(cfg->text.data, cfg->text.length);
    set_display_filename(v->buffer, xasprintf("(builtin %s)", name));
}

static void show_option(const char *name, bool cflag)
{
    const char *value = get_option_value_string(name);
    if (!value) {
        error_msg("invalid option name: %s", name);
        return;
    }
    if (cflag) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, value);
    } else {
        info_msg("%s is set to: %s", name, value);
    }
}

static void show_wsplit(const char *name, bool cflag)
{
    if (!streq(name, "this")) {
        error_msg("invalid window: %s", name);
        return;
    }

    const Window *w = window;
    char buf[128];
    xsnprintf(buf, sizeof buf, "%d,%d %dx%d", w->x, w->y, w->w, w->h);

    if (cflag) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, buf);
    } else {
        info_msg("current window dimensions: %s", buf);
    }
}

static void cmd_show(const CommandArgs *a)
{
    static const struct {
        const char name[8];
        void (*show)(const char *name, bool cmdline);
        String (*dump)(void);
    } handlers[] = {
        {"alias", show_alias, dump_aliases},
        {"bind", show_binding, dump_bindings},
        {"color", show_color, dump_hl_colors},
        {"include", show_include, dump_builtin_configs},
        {"option", show_option, dump_options},
        {"wsplit", show_wsplit, dump_frames},
    };

    ssize_t cmdtype = -1;
    for (ssize_t i = 0; i < ARRAY_COUNT(handlers); i++) {
        if (streq(a->args[0], handlers[i].name)) {
            cmdtype = i;
            break;
        }
    }
    if (cmdtype < 0) {
        error_msg("invalid argument: '%s'", a->args[0]);
        return;
    }

    const bool cflag = a->nr_flags != 0;
    if (a->nr_args == 2) {
        handlers[cmdtype].show(a->args[1], cflag);
        return;
    }
    if (cflag) {
        error_msg("\"show -c\" requires 2 arguments");
        return;
    }

    String s = handlers[cmdtype].dump();
    View *v = window_open_new_file(window);
    v->buffer->temporary = true;
    do_insert(s.buffer, s.len);
    string_free(&s);
    set_display_filename(v->buffer, xasprintf("(show %s)", a->args[0]));
    v->buffer->encoding = encoding_from_type(UTF8);
}

static void cmd_suspend(const CommandArgs* UNUSED_ARG(a))
{
    suspend();
}

static void cmd_tag(const CommandArgs *a)
{
    do_selection(SELECT_NONE);

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

static void cmd_title(const CommandArgs *a)
{
    if (buffer->abs_filename) {
        error_msg("saved buffers can't be retitled");
        return;
    }
    set_display_filename(buffer, xstrdup(a->args[0]));
    mark_buffer_tabbars_changed(buffer);
}

static void cmd_toggle(const CommandArgs *a)
{
    bool global = has_flag(a, 'g');
    bool verbose = has_flag(a, 'v');
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
    handle_select_chars_or_lines_flags(a);
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
    bool force = has_flag(a, 'f');
    bool prompt = has_flag(a, 'p');
    View *v = window_find_unclosable_view(window);
    if (v && !force) {
        set_view(v);
        if (prompt) {
            if (dialog_prompt("Close window without saving? [y/N]", "ny") != 'y') {
                return;
            }
        } else {
            error_msg (
                "Save modified files or run 'wclose -f' to close "
                "window without saving."
            );
            return;
        }
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
    handle_select_chars_flag(a);
    bool skip_non_word = has_flag(a, 's');
    word_bwd(&view->cursor, skip_non_word);
    view_reset_preferred_x(view);
}

static void cmd_word_fwd(const CommandArgs *a)
{
    handle_select_chars_flag(a);
    bool skip_non_word = has_flag(a, 's');
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

    ResizeDirection dir = RESIZE_DIRECTION_AUTO;
    if (a->nr_flags) {
        switch (a->flags[a->nr_flags - 1]) {
        case 'h':
            dir = RESIZE_DIRECTION_HORIZONTAL;
            break;
        case 'v':
            dir = RESIZE_DIRECTION_VERTICAL;
            break;
        }
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
    bool before = false;
    bool use_glob = false;
    bool vertical = false;
    bool root = false;
    bool empty = false;
    bool temporary = false;
    for (const char *pf = a->flags; *pf; pf++) {
        switch (*pf) {
        case 'b':
            // Add new window before current window
            before = true;
            break;
        case 'e':
            empty = true;
            break;
        case 'g':
            // Perform glob(3) expansion on arguments
            use_glob = !!a->args[0];
            break;
        case 'h':
            // Split horizontally to get vertical layout
            vertical = true;
            break;
        case 'r':
            // Split root frame instead of current window
            root = true;
            break;
        case 't':
            temporary = true;
            empty = true;
            break;
        }
    }

    if (empty && a->nr_args > 0) {
        error_msg("flags -e and -t can't be used with filename arguments");
        return;
    }

    char **paths = a->args;
    glob_t globbuf;
    if (use_glob) {
        if (!xglob(a->args, &globbuf)) {
            return;
        }
        paths = globbuf.gl_pathv;
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

    if (empty) {
        window_open_new_file(window);
        buffer->temporary = temporary;
    } else if (paths[0]) {
        window_open_files(window, paths, NULL);
    } else {
        View *new = window_add_buffer(window, save->buffer);
        new->cursor = save->cursor;
        set_view(new);
    }

    if (use_glob) {
        globfree(&globbuf);
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

static const Command cmds[] = {
    {"alias", "", 2, 2, cmd_alias},
    {"bind", "", 1, 2, cmd_bind},
    {"blkdown", "cl", 0, 0, cmd_blkdown},
    {"blkup", "cl", 0, 0, cmd_blkup},
    {"bof", "", 0, 0, cmd_bof},
    {"bol", "cs", 0, 0, cmd_bol},
    {"bolsf", "", 0, 0, cmd_bolsf},
    {"case", "lu", 0, 0, cmd_case},
    {"cd", "", 1, 1, cmd_cd},
    {"center-view", "", 0, 0, cmd_center_view},
    {"clear", "", 0, 0, cmd_clear},
    {"close", "fpqw", 0, 0, cmd_close},
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
    {"exec-open", "-s", 1, -1, cmd_exec_open},
    {"filter", "-l", 1, -1, cmd_filter},
    {"ft", "-bcfi", 2, -1, cmd_ft},
    {"hi", "-", 0, -1, cmd_hi},
    {"include", "bq", 1, 1, cmd_include},
    {"insert", "km", 1, 1, cmd_insert},
    {"join", "", 0, 0, cmd_join},
    {"left", "c", 0, 0, cmd_left},
    {"line", "", 1, 1, cmd_line},
    {"load-syntax", "", 1, 1, cmd_load_syntax},
    {"move-tab", "", 1, 1, cmd_move_tab},
    {"msg", "np", 0, 0, cmd_msg},
    {"new-line", "", 0, 0, cmd_new_line},
    {"next", "", 0, 0, cmd_next},
    {"open", "e=gt", 0, -1, cmd_open},
    {"option", "-r", 3, -1, cmd_option},
    {"paste", "c", 0, 0, cmd_paste},
    {"pgdown", "cl", 0, 0, cmd_pgdown},
    {"pgup", "cl", 0, 0, cmd_pgup},
    {"pipe-from", "-ms", 1, -1, cmd_pipe_from},
    {"pipe-to", "-l", 1, -1, cmd_pipe_to},
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
    {"title", "", 1, 1, cmd_title},
    {"toggle", "glv", 1, -1, cmd_toggle},
    {"undo", "", 0, 0, cmd_undo},
    {"unselect", "", 0, 0, cmd_unselect},
    {"up", "cl", 0, 0, cmd_up},
    {"view", "", 1, 1, cmd_view},
    {"wclose", "fp", 0, 0, cmd_wclose},
    {"wflip", "", 0, 0, cmd_wflip},
    {"wnext", "", 0, 0, cmd_wnext},
    {"word-bwd", "cs", 0, 0, cmd_word_bwd},
    {"word-fwd", "cs", 0, 0, cmd_word_fwd},
    {"wprev", "", 0, 0, cmd_wprev},
    {"wrap-paragraph", "", 0, 1, cmd_wrap_paragraph},
    {"wresize", "hv", 0, 1, cmd_wresize},
    {"wsplit", "beghrt", 0, -1, cmd_wsplit},
    {"wswap", "", 0, 0, cmd_wswap},
};

const Command *find_normal_command(const char *name)
{
    return bsearch(name, cmds, ARRAY_COUNT(cmds), sizeof(cmds[0]), command_cmp);
}

const CommandSet commands = {
    .lookup = find_normal_command
};

void collect_normal_commands(const char *prefix)
{
    for (size_t i = 0, n = ARRAY_COUNT(cmds); i < n; i++) {
        const Command *c = &cmds[i];
        if (str_has_prefix(c->name, prefix)) {
            add_completion(xstrdup(c->name));
        }
    }
    collect_aliases(prefix);
}

UNITTEST {
    for (size_t i = 1, n = ARRAY_COUNT(cmds); i < n; i++) {
        // Check that fixed-size arrays are null-terminated within bounds
        const Command *const cmd = &cmds[i];
        BUG_ON(cmd->name[ARRAY_COUNT(cmds[0].name) - 1] != '\0');
        BUG_ON(cmd->flags[ARRAY_COUNT(cmds[0].flags) - 1] != '\0');

        // Check that array is sorted by name field, in binary searchable order
        BUG_ON(strcmp(cmd->name, cmds[i - 1].name) <= 0);

        // Count number of real flags (i.e. not including '-' or '=')
        size_t nr_real_flags = 0;
        for (size_t j = 0; cmd->flags[j]; j++) {
            if (ascii_isalnum(cmd->flags[j])) {
                nr_real_flags++;
            }
        }

        // Check that max. number of real flags fits in CommandArgs::flags
        // array (and also leaves 1 byte for null-terminator)
        CommandArgs a;
        BUG_ON(nr_real_flags >= ARRAY_COUNT(a.flags));
    }
}
