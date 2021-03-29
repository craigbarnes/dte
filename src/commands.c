#include <errno.h>
#include <glob.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "commands.h"
#include "alias.h"
#include "bind.h"
#include "change.h"
#include "cmdline.h"
#include "command/args.h"
#include "command/macro.h"
#include "command/serialize.h"
#include "completion.h"
#include "config.h"
#include "convert.h"
#include "edit.h"
#include "editor.h"
#include "encoding.h"
#include "error.h"
#include "file-option.h"
#include "filetype.h"
#include "frame.h"
#include "history.h"
#include "load-save.h"
#include "lock.h"
#include "move.h"
#include "msg.h"
#include "regexp.h"
#include "search.h"
#include "selection.h"
#include "show.h"
#include "spawn.h"
#include "syntax/state.h"
#include "syntax/syntax.h"
#include "tag.h"
#include "terminal/color.h"
#include "terminal/terminal.h"
#include "util/bsearch.h"
#include "util/checked-arith.h"
#include "util/debug.h"
#include "util/path.h"
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
    buffer_mark_lines_changed(buffer, view->cy, view->cy);
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
    const char *const name = a->args[0];
    if (unlikely(name[0] == '\0')) {
        error_msg("Empty alias name not allowed");
        return;
    }

    for (size_t i = 0; name[i]; i++) {
        unsigned char c = name[i];
        if (unlikely(!(is_word_byte(c) || c == '-' || c == '?' || c == '!'))) {
            error_msg("Invalid byte in alias name: %c (0x%02hhX)", c, c);
            return;
        }
    }

    if (unlikely(find_normal_command(name))) {
        error_msg("Can't replace existing command %s with an alias", name);
        return;
    }

    add_alias(name, a->args[1]);
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

static void cmd_bookmark(const CommandArgs *a)
{
    if (has_flag(a, 'r')) {
        pop_file_location();
        return;
    }

    push_file_location(get_current_file_location());
}

static void cmd_case(const CommandArgs *a)
{
    change_case(last_flag_or_default(a, 't'));
}

static void cmd_cd(const CommandArgs *a)
{
    const char *dir = a->args[0];
    char cwd[8192];
    const char *cwdp = NULL;
    bool got_cwd = !!getcwd(cwd, sizeof(cwd));

    if (streq(dir, "-")) {
        dir = getenv("OLDPWD");
        if (!dir || dir[0] == '\0') {
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
    bool force = has_flag(a, 'f');
    bool prompt = has_flag(a, 'p');
    bool allow_quit = has_flag(a, 'q');
    bool allow_wclose = has_flag(a, 'w');

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
        activate_current_message_save();
    }
}

static void cmd_copy(const CommandArgs *a)
{
    BlockIter save = view->cursor;
    if (view->selection) {
        copy(prepare_selection(view), view->selection == SELECT_LINES);
        bool keep_selection = has_flag(a, 'k');
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

    bool delete_newline_if_at_eol = has_flag(a, 'n');
    BlockIter bi = view->cursor;
    if (delete_newline_if_at_eol) {
        CodePoint ch;
        if (block_iter_get_char(&view->cursor, &ch) == 1 && ch == '\n') {
            delete_ch();
            return;
        }
    }

    buffer_delete_bytes(block_iter_eol(&bi));
}

static void cmd_delete_line(const CommandArgs* UNUSED_ARG(a))
{
    delete_lines();
}

static void cmd_delete_word(const CommandArgs *a)
{
    bool skip_non_word = has_flag(a, 's');
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
    bool skip_non_word = has_flag(a, 's');
    buffer_erase_bytes(word_bwd(&view->cursor, skip_non_word));
}

static void cmd_errorfmt(const CommandArgs *a)
{
    bool ignore = has_flag(a, 'i');
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
    macro_command_hook("open", (char**)filenames.ptrs);
    ptr_array_free_array(&filenames);
    string_free(&ctx.output);
}

static void cmd_exec_tag(const CommandArgs *a)
{
    SpawnContext ctx = {
        .argv = a->args,
        .output = STRING_INIT,
        .flags = has_flag(a, 's') ? SPAWN_QUIET : SPAWN_DEFAULT
    };
    if (!spawn_source(&ctx)) {
        return;
    }

    const char *tag = string_borrow_cstring(&ctx.output);
    const char *nl = memchr(tag, '\n', ctx.output.len);
    size_t tag_len = nl ? (size_t)(nl - tag) : ctx.output.len;

    String s = string_new(tag_len + 16);
    string_append_literal(&s, "tag ");
    if (unlikely(tag[0] == '-')) {
        string_append_literal(&s, "-- ");
    }
    string_append_escaped_arg_sv(&s, string_view(tag, tag_len), true);
    string_free(&ctx.output);
    handle_command(&commands, string_borrow_cstring(&s), true);
    string_free(&s);
}

static const char **lines_and_columns_env(void)
{
    static char lines[DECIMAL_STR_MAX(window->edit_h)];
    static char columns[DECIMAL_STR_MAX(window->edit_w)];
    static const char *vars[] = {
        "LINES", lines,
        "COLUMNS", columns,
        NULL,
    };

    xsnprintf(lines, sizeof lines, "%d", window->edit_h);
    xsnprintf(columns, sizeof columns, "%d", window->edit_w);
    return vars;
}

static void cmd_filter(const CommandArgs *a)
{
    BlockIter save = view->cursor;
    SpawnContext ctx = {
        .argv = a->args,
        .env = lines_and_columns_env(),
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
    char **args = a->args;
    const char *filetype = args[0];
    if (unlikely(filetype[0] == '\0')) {
        error_msg("Filetype can't be blank");
        return;
    }

    FileDetectionType dt = FT_EXTENSION;
    switch (last_flag(a)) {
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

    for (size_t i = 1, n = a->nr_args; i < n; i++) {
        add_filetype(filetype, args[i], dt);
    }
}

static void cmd_hi(const CommandArgs *a)
{
    if (unlikely(a->nr_args == 0)) {
        exec_builtin_color_reset();
        goto update;
    }

    TermColor color;
    if (unlikely(!parse_term_color(&color, a->args + 1))) {
        return;
    }

    int32_t fg = color_to_nearest(color.fg, terminal.color_type);
    int32_t bg = color_to_nearest(color.bg, terminal.color_type);
    if (
        terminal.color_type != TERM_TRUE_COLOR
        && has_flag(a, 'c')
        && (fg != color.fg || bg != color.bg)
    ) {
        return;
    }

    color.fg = fg;
    color.bg = bg;
    set_highlight_color(a->args[0], &color);

update:
    // Don't call update_all_syntax_colors() needlessly.
    // It is called right after config has been loaded.
    if (editor.status != EDITOR_INITIALIZING) {
        update_all_syntax_colors();
        mark_everything_changed();
    }
}

static void cmd_include(const CommandArgs *a)
{
    ConfigFlags flags = has_flag(a, 'q') ? CFG_NOFLAGS : CFG_MUST_EXIST;
    if (has_flag(a, 'b')) {
        flags |= CFG_BUILTIN;
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

    bool move_after = has_flag(a, 'm');
    insert_text(str, strlen(str), move_after);
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

static void cmd_macro(const CommandArgs *a)
{
    static const struct {
        const char name[8];
        void (*handler)(void);
    } actions[] = {
        {"record", macro_record},
        {"stop", macro_stop},
        {"toggle", macro_toggle},
        {"cancel", macro_cancel},
        {"play", macro_play},
        {"run", macro_play},
    };
    const char *action = a->args[0];
    for (size_t i = 0; i < ARRAY_COUNT(actions); i++) {
        if (streq(action, actions[i].name)) {
            actions[i].handler();
            return;
        }
    }
    error_msg("Unknown action '%s'", action);
}

static void cmd_match_bracket(const CommandArgs* UNUSED_ARG(a))
{
    CodePoint cursor_char;
    if (!block_iter_get_char(&view->cursor, &cursor_char)) {
        error_msg("No character under cursor");
        return;
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
        error_msg("Character under cursor not matchable");
        return;
    }

search_fwd:
    block_iter_next_char(&bi, &u);
    BUG_ON(u != cursor_char);
    while (block_iter_next_char(&bi, &u)) {
        if (u == target) {
            if (level == 0) {
                bi.offset--;
                view->cursor = bi;
                return; // Found
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
                view->cursor = bi;
                return; // Found
            }
            level--;
        } else if (u == cursor_char) {
            level++;
        }
    }

not_found:
    error_msg("No matching bracket found");
}

static void cmd_move_tab(const CommandArgs *a)
{
    const size_t ntabs = window->views.count;
    const char *str = a->args[0];
    size_t to, from = ptr_array_idx(&window->views, view);
    BUG_ON(from >= ntabs);
    if (streq(str, "left")) {
        to = (from ? from : ntabs) - 1;
    } else if (streq(str, "right")) {
        to = (from + 1) % ntabs;
    } else {
        if (!str_to_size(str, &to) || to == 0) {
            error_msg("Invalid tab position %s", str);
            return;
        }
        to--;
        if (to >= ntabs) {
            to = ntabs - 1;
        }
    }
    ptr_array_move(&window->views, from, to);
    window->update_tabbar = true;
}

static void cmd_msg(const CommandArgs *a)
{
    switch (last_flag(a)) {
    case 'n':
        activate_next_message();
        return;
    case 'p':
        activate_prev_message();
        return;
    }
    activate_current_message();
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
    error_msg("glob: %s", (err == GLOB_NOSPACE) ? strerror(ENOMEM) : "failed");
    globfree(globbuf);
    return false;
}

static void cmd_open(const CommandArgs *a)
{
    bool temporary = has_flag(a, 't');
    if (unlikely(temporary && a->nr_args > 0)) {
        error_msg("'open -t' can't be used with filename arguments");
        return;
    }

    const char *requested_encoding = NULL;
    char **args = a->args;
    if (a->nr_flag_args > 0) {
        // The "-e" flag is the only one that takes an argument, so the
        // above condition implies it was used
        BUG_ON(!has_flag(a, 'e'));
        requested_encoding = args[a->nr_flag_args - 1];
        args += a->nr_flag_args;
    }

    Encoding encoding;
    if (requested_encoding) {
        EncodingType e = lookup_encoding(requested_encoding);
        if (e == UTF8) {
            encoding = encoding_from_type(e);
        } else if (conversion_supported_by_iconv(requested_encoding, "UTF-8")) {
            encoding = encoding_from_name(requested_encoding);
        } else {
            if (errno == EINVAL) {
                error_msg("Unsupported encoding '%s'", requested_encoding);
            } else {
                error_msg (
                    "iconv conversion from '%s' failed: %s",
                    requested_encoding,
                    strerror(errno)
                );
            }
            return;
        }
    } else {
        encoding = (Encoding){.type = ENCODING_AUTODETECT};
    }

    if (a->nr_args == 0) {
        View *v = window_open_new_file(window);
        v->buffer->temporary = temporary;
        if (requested_encoding) {
            buffer_set_encoding(v->buffer, encoding);
        }
        return;
    }

    char **paths = args;
    glob_t globbuf;
    bool use_glob = has_flag(a, 'g');
    if (use_glob) {
        if (!xglob(args, &globbuf)) {
            return;
        }
        paths = globbuf.gl_pathv;
    }

    if (!paths[1]) {
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
    BUG_ON(a->nr_args < 3);
    size_t nstrs = a->nr_args - 1;
    if (unlikely(nstrs % 2 != 0)) {
        error_msg("Missing option value");
        return;
    }

    char **strs = a->args + 1;
    if (unlikely(!validate_local_options(strs))) {
        return;
    }

    if (has_flag(a, 'r')) {
        char *regex = xstrdup(a->args[0]);
        char **opts = copy_string_array(strs, nstrs);
        add_file_options(FILE_OPTIONS_FILENAME, regex, opts);
        return;
    }

    const char *ft_list = a->args[0];
    for (size_t pos = 0, len = strlen(ft_list); pos < len; ) {
        const StringView sv = get_delim(ft_list, &pos, len, ',');
        char *filetype = xstrcut(sv.data, sv.length);
        char **opts = copy_string_array(strs, nstrs);
        add_file_options(FILE_OPTIONS_FILETYPE, filetype, opts);
    }
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
    bool at_cursor = has_flag(a, 'c');
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
    SpawnContext ctx = {
        .argv = a->args,
        .env = lines_and_columns_env(),
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

    bool strip_nl = has_flag(a, 's');
    bool move = has_flag(a, 'm');
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
    int exit_code = 0;
    if (a->nr_args) {
        if (!str_to_int(a->args[0], &exit_code)) {
            error_msg("Not a valid integer argument: '%s'", a->args[0]);
            return;
        }
        if (exit_code < 0 || exit_code > 125) {
            error_msg("Exit code should be between 0 and 125");
            return;
        }
    }

    if (has_flag(a, 'f')) {
        goto exit;
    }

    for (size_t i = 0, n = editor.buffers.count; i < n; i++) {
        Buffer *b = editor.buffers.ptrs[i];
        if (buffer_modified(b)) {
            // Activate modified buffer
            View *v = window_find_view(window, b);
            if (!v) {
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
    editor.exit_code = exit_code;
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
    unsigned int count = 0;
    if (!str_to_uint(a->args[0], &count)) {
        error_msg("Not a valid repeat count: %s", a->args[0]);
        return;
    } else if (count == 0) {
        return;
    }

    const Command *cmd = find_normal_command(a->args[1]);
    if (!cmd) {
        error_msg("No such command: %s", a->args[1]);
        return;
    }

    CommandArgs a2 = {.args = a->args + 2};
    current_command = cmd;
    bool ok = parse_args(cmd, &a2);
    current_command = NULL;
    if (!ok) {
        return;
    }

    // Optimized special case for "insert" command
    if (cmd->cmd == cmd_insert && !has_flag(&a2, 'k')) {
        const char *str = a2.args[0];
        size_t str_len = strlen(str);
        size_t bufsize;
        if (size_multiply_overflows(count, str_len, &bufsize)) {
            error_msg("Repeated insert would overflow");
            return;
        }
        char *buf = malloc(bufsize);
        if (!buf) {
            perror_msg("malloc");
            return;
        }
        // TODO: for many repeats of small strings, fill in the first 4K
        // and then use that to batch copy larger blocks (making sure to
        // handle any unaligned remainder).
        for (size_t i = 0; i < count; i++) {
            memcpy(buf + (i * str_len), str, str_len);
        }
        bool move_after = has_flag(&a2, 'm');
        insert_text(buf, bufsize, move_after);
        free(buf);
        return;
    }

    while (count--) {
        cmd->cmd(&a2);
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
    if (unlikely(buffer->stdout_buffer)) {
        const char *f = buffer_filename(buffer);
        info_msg("%s can't be saved; it will be piped to stdout on exit", f);
        return;
    }

    bool dos_nl = has_flag(a, 'd');
    bool unix_nl = has_flag(a, 'u');
    bool crlf = buffer->crlf_newlines;
    if (unlikely(dos_nl && unix_nl)) {
        error_msg("flags -d and -u can't be used together");
        return;
    } else if (dos_nl) {
        crlf = true;
    } else if (unix_nl) {
        crlf = false;
    }

    const char *requested_encoding = NULL;
    char **args = a->args;
    if (a->nr_flag_args > 0) {
        BUG_ON(!has_flag(a, 'e'));
        requested_encoding = args[a->nr_flag_args - 1];
        args += a->nr_flag_args;
    }

    Encoding encoding = buffer->encoding;
    bool bom = buffer->bom;
    if (requested_encoding) {
        EncodingType e = lookup_encoding(requested_encoding);
        if (e == UTF8) {
            if (encoding.type != UTF8) {
                // Encoding changed
                encoding = encoding_from_type(e);
                bom = editor.options.utf8_bom;
            }
        } else if (conversion_supported_by_iconv("UTF-8", requested_encoding)) {
            encoding = encoding_from_name(requested_encoding);
            if (encoding.name != buffer->encoding.name) {
                // Encoding changed
                bom = !!get_bom_for_encoding(encoding.type);
            }
        } else {
            if (errno == EINVAL) {
                error_msg("Unsupported encoding '%s'", requested_encoding);
            } else {
                error_msg (
                    "iconv conversion to '%s' failed: %s",
                    requested_encoding,
                    strerror(errno)
                );
            }
            return;
        }
    }

    bool b = has_flag(a, 'b');
    bool B = has_flag(a, 'B');
    if (unlikely(b && B)) {
        error_msg("flags -b and -B can't be used together");
        return;
    } else if (b) {
        bom = true;
    } else if (B) {
        bom = false;
    }

    char *absolute = buffer->abs_filename;
    bool force = has_flag(a, 'f');
    bool new_locked = false;
    if (a->nr_args > 0) {
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
            bool prompt = has_flag(a, 'p');
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

    mode_t old_mode = buffer->file.mode;
    struct stat st;
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

    if (!save_buffer(buffer, absolute, &encoding, crlf, bom)) {
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
    bool history = !has_flag(a, 'H');
    bool next = has_flag(a, 'n');
    bool prev = has_flag(a, 'p');
    bool w = has_flag(a, 'w');
    SearchDirection dir = has_flag(a, 'r') ? SEARCH_BWD : SEARCH_FWD;

    if (unlikely(w && pattern)) {
        error_msg("flag -w can't be used with search pattern.");
        return;
    }
    if (unlikely(next && prev)) {
        error_msg("flags -n and -p can't be used together");
        return;
    }

    char pattbuf[4096];
    if (w) {
        StringView word = view_get_word_under_cursor(view);
        if (word.length == 0) {
            // Error message would not be very useful here
            return;
        }
        const size_t bmax = sizeof(regexp_word_boundary.start);
        static_assert_compatible_types(regexp_word_boundary.start, char[8]);
        if (unlikely(word.length >= sizeof(pattbuf) - (bmax * 2))) {
            error_msg("word under cursor too long");
            return;
        }
        char *ptr = stpncpy(pattbuf, regexp_word_boundary.start, bmax);
        memcpy(ptr, word.data, word.length);
        memcpy(ptr + word.length, regexp_word_boundary.end, bmax);
        pattern = pattbuf;
    }

    do_selection(SELECT_NONE);

    if (pattern) {
        set_search_direction(dir);
        search_set_regexp(pattern);
        if (w) {
            search_next_word();
        } else {
            search_next();
        }
        if (history) {
            history_add(&editor.search_history, pattern);
        }
    } else if (next) {
        search_next();
    } else if (prev) {
        search_prev();
    } else {
        set_input_mode(INPUT_SEARCH);
        set_search_direction(dir);
    }
}

static void cmd_select(const CommandArgs *a)
{
    SelectionType sel = has_flag(a, 'l') ? SELECT_LINES : SELECT_CHARS;
    bool block = has_flag(a, 'b');
    bool keep = has_flag(a, 'k');
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
    buffer_mark_lines_changed(buffer, view->cy, view->cy);
}

static void cmd_set(const CommandArgs *a)
{
    bool global = has_flag(a, 'g');
    bool local = has_flag(a, 'l');
    if (!buffer) {
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
    if (unlikely(streq(name, "DTE_VERSION"))) {
        error_msg("$DTE_VERSION cannot be changed");
        return;
    }

    const size_t nr_args = a->nr_args;
    int res;
    if (nr_args == 2) {
        res = setenv(name, a->args[1], true);
    } else {
        BUG_ON(nr_args != 1);
        res = unsetenv(name);
    }

    if (unlikely(res != 0)) {
        if (errno == EINVAL) {
            error_msg("Invalid environment variable name '%s'", name);
        } else {
            perror_msg(nr_args == 2 ? "setenv" : "unsetenv");
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

static void cmd_show(const CommandArgs *a)
{
    BUG_ON(a->nr_args == 1 && a->args[1]);
    bool cflag = a->nr_flags != 0;
    if (cflag && a->nr_args < 2) {
        error_msg("\"show -c\" requires 2 arguments");
        return;
    }
    show(a->args[0], a->args[1], cflag);
}

static void cmd_suspend(const CommandArgs* UNUSED_ARG(a))
{
    suspend();
}

static void cmd_tag(const CommandArgs *a)
{
    if (has_flag(a, 'r')) {
        pop_file_location();
        return;
    }

    clear_messages();
    TagFile *tf = load_tag_file();
    if (!tf) {
        error_msg("No tag file.");
        return;
    }

    const char *name = a->args[0];
    char *word = NULL;
    if (!name) {
        StringView w = view_get_word_under_cursor(view);
        if (w.length == 0) {
            return;
        }
        word = xstrcut(w.data, w.length);
        name = word;
    }

    PointerArray tags = PTR_ARRAY_INIT;
    // Filename helps to find correct tags
    tag_file_find_tags(tf, buffer->abs_filename, name, &tags);
    if (tags.count == 0) {
        error_msg("Tag '%s' not found", name);
    } else {
        for (size_t i = 0, n = tags.count; i < n; i++) {
            Tag *t = tags.ptrs[i];
            char buf[512];
            size_t len = xsnprintf(buf, sizeof(buf), "Tag %s", name);
            Message *m = new_message(buf, len);
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
    if (!f->parent) {
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
    if (!window->frame->parent) {
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
    bool before = has_flag(a, 'b');
    bool use_glob = has_flag(a, 'g') && a->nr_args > 0;
    bool vertical = has_flag(a, 'h');
    bool root = has_flag(a, 'r');
    bool temporary = has_flag(a, 't');
    bool empty = temporary || has_flag(a, 'n');

    if (empty && a->nr_args > 0) {
        error_msg("flags -n and -t can't be used with filename arguments");
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
    if (!parent) {
        return;
    }

    size_t i = ptr_array_idx(&parent->frames, window->frame);
    BUG_ON(i >= parent->frames.count);
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
    {"alias", "-", true, 2, 2, cmd_alias},
    {"bind", "-", true, 1, 2, cmd_bind},
    {"blkdown", "cl", false, 0, 0, cmd_blkdown},
    {"blkup", "cl", false, 0, 0, cmd_blkup},
    {"bof", "", false, 0, 0, cmd_bof},
    {"bol", "cs", false, 0, 0, cmd_bol},
    {"bolsf", "", false, 0, 0, cmd_bolsf},
    {"bookmark", "r", false, 0, 0, cmd_bookmark},
    {"case", "lu", false, 0, 0, cmd_case},
    {"cd", "", true, 1, 1, cmd_cd},
    {"center-view", "", false, 0, 0, cmd_center_view},
    {"clear", "", false, 0, 0, cmd_clear},
    {"close", "fpqw", false, 0, 0, cmd_close},
    {"command", "-", false, 0, 1, cmd_command},
    {"compile", "-1ps", false, 2, -1, cmd_compile},
    {"copy", "k", false, 0, 0, cmd_copy},
    {"cut", "", false, 0, 0, cmd_cut},
    {"delete", "", false, 0, 0, cmd_delete},
    {"delete-eol", "n", false, 0, 0, cmd_delete_eol},
    {"delete-line", "", false, 0, 0, cmd_delete_line},
    {"delete-word", "s", false, 0, 0, cmd_delete_word},
    {"down", "cl", false, 0, 0, cmd_down},
    {"eof", "", false, 0, 0, cmd_eof},
    {"eol", "c", false, 0, 0, cmd_eol},
    {"eolsf", "", false, 0, 0, cmd_eolsf},
    {"erase", "", false, 0, 0, cmd_erase},
    {"erase-bol", "", false, 0, 0, cmd_erase_bol},
    {"erase-word", "s", false, 0, 0, cmd_erase_word},
    {"errorfmt", "i", true, 2, 18, cmd_errorfmt},
    {"eval", "-", false, 1, -1, cmd_eval},
    {"exec-open", "-s", false, 1, -1, cmd_exec_open},
    {"exec-tag", "-s", false, 1, -1, cmd_exec_tag},
    {"filter", "-l", false, 1, -1, cmd_filter},
    {"ft", "-bcfi", true, 2, -1, cmd_ft},
    {"hi", "-c", true, 0, -1, cmd_hi},
    {"include", "bq", true, 1, 1, cmd_include},
    {"insert", "km", false, 1, 1, cmd_insert},
    {"join", "", false, 0, 0, cmd_join},
    {"left", "c", false, 0, 0, cmd_left},
    {"line", "", false, 1, 1, cmd_line},
    {"load-syntax", "", true, 1, 1, cmd_load_syntax},
    {"macro", "", false, 1, 1, cmd_macro},
    {"match-bracket", "", false, 0, 0, cmd_match_bracket},
    {"move-tab", "", false, 1, 1, cmd_move_tab},
    {"msg", "np", false, 0, 0, cmd_msg},
    {"new-line", "", false, 0, 0, cmd_new_line},
    {"next", "", false, 0, 0, cmd_next},
    {"open", "e=gt", false, 0, -1, cmd_open},
    {"option", "-r", true, 3, -1, cmd_option},
    {"paste", "c", false, 0, 0, cmd_paste},
    {"pgdown", "cl", false, 0, 0, cmd_pgdown},
    {"pgup", "cl", false, 0, 0, cmd_pgup},
    {"pipe-from", "-ms", false, 1, -1, cmd_pipe_from},
    {"pipe-to", "-l", false, 1, -1, cmd_pipe_to},
    {"prev", "", false, 0, 0, cmd_prev},
    {"quit", "fp", false, 0, 1, cmd_quit},
    {"redo", "", false, 0, 1, cmd_redo},
    {"refresh", "", false, 0, 0, cmd_refresh},
    {"repeat", "-", false, 2, -1, cmd_repeat},
    {"replace", "bcgi", false, 2, 2, cmd_replace},
    {"right", "c", false, 0, 0, cmd_right},
    {"run", "-ps", false, 1, -1, cmd_run},
    {"save", "Bbde=fpu", false, 0, 1, cmd_save},
    {"scroll-down", "", false, 0, 0, cmd_scroll_down},
    {"scroll-pgdown", "", false, 0, 0, cmd_scroll_pgdown},
    {"scroll-pgup", "", false, 0, 0, cmd_scroll_pgup},
    {"scroll-up", "", false, 0, 0, cmd_scroll_up},
    {"search", "Hnprw", false, 0, 1, cmd_search},
    {"select", "bkl", false, 0, 0, cmd_select},
    {"set", "gl", true, 1, -1, cmd_set},
    {"setenv", "", true, 1, 2, cmd_setenv},
    {"shift", "", false, 1, 1, cmd_shift},
    {"show", "c", false, 1, 2, cmd_show},
    {"suspend", "", false, 0, 0, cmd_suspend},
    {"tag", "r", false, 0, 1, cmd_tag},
    {"title", "", false, 1, 1, cmd_title},
    {"toggle", "glv", false, 1, -1, cmd_toggle},
    {"undo", "", false, 0, 0, cmd_undo},
    {"unselect", "", false, 0, 0, cmd_unselect},
    {"up", "cl", false, 0, 0, cmd_up},
    {"view", "", false, 1, 1, cmd_view},
    {"wclose", "fp", false, 0, 0, cmd_wclose},
    {"wflip", "", false, 0, 0, cmd_wflip},
    {"wnext", "", false, 0, 0, cmd_wnext},
    {"word-bwd", "cs", false, 0, 0, cmd_word_bwd},
    {"word-fwd", "cs", false, 0, 0, cmd_word_fwd},
    {"wprev", "", false, 0, 0, cmd_wprev},
    {"wrap-paragraph", "", false, 0, 1, cmd_wrap_paragraph},
    {"wresize", "hv", false, 0, 1, cmd_wresize},
    {"wsplit", "bghnrt", false, 0, -1, cmd_wsplit},
    {"wswap", "", false, 0, 0, cmd_wswap},
};

static bool allow_macro_recording(const Command *cmd, char **args)
{
    IGNORE_WARNING("-Wpedantic")
    static const void *const non_recordable[] = {
        cmd_macro, cmd_command, cmd_exec_open, cmd_exec_tag,
    };
    for (size_t i = 0; i < ARRAY_COUNT(non_recordable); i++) {
        if (cmd->cmd == non_recordable[i]) {
            return false;
        }
    }
    UNIGNORE_WARNINGS

    if (cmd->cmd == cmd_search) {
        char **args_copy = copy_string_array(args, string_array_length(args));
        CommandArgs a = {.args = args_copy};
        bool ret = true;
        if (do_parse_args(cmd, &a) == 0) {
            if (a.nr_args == 0 && !strpbrk(a.flags, "npw")) {
                // If command is "search" with no pattern argument and without
                // flags -n, -p or -w, the command would put the editor into
                // search mode, which shouldn't be recorded.
                ret = false;
            }
        }
        free_string_array(args_copy);
        return ret;
    }
    return true;
}

const Command *find_normal_command(const char *name)
{
    return BSEARCH(name, cmds, command_cmp);
}

const CommandSet commands = {
    .lookup = find_normal_command,
    .allow_recording = allow_macro_recording,
};

void collect_normal_commands(const char *prefix)
{
    for (size_t i = 0, n = ARRAY_COUNT(cmds); i < n; i++) {
        const Command *c = &cmds[i];
        if (str_has_prefix(c->name, prefix)) {
            add_completion(xstrdup(c->name));
        }
    }
}

UNITTEST {
    CHECK_BSEARCH_ARRAY(cmds, name, strcmp);

    for (size_t i = 0, n = ARRAY_COUNT(cmds); i < n; i++) {
        // Check that flags arrays is null-terminated within bounds
        const char *const flags = cmds[i].flags;
        BUG_ON(flags[ARRAY_COUNT(cmds[0].flags) - 1] != '\0');

        // Count number of real flags (i.e. not including '-' or '=')
        size_t nr_real_flags = 0;
        for (size_t j = (flags[0] == '-' ? 1 : 0); flags[j]; j++) {
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
        BUG_ON(nr_real_flags >= ARRAY_COUNT(a.flags));
    }
}
