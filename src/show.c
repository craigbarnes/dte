#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "show.h"
#include "bind.h"
#include "buffer.h"
#include "change.h"
#include "cmdline.h"
#include "command/alias.h"
#include "command/macro.h"
#include "command/serialize.h"
#include "commands.h"
#include "compiler.h"
#include "completion.h"
#include "config.h"
#include "edit.h"
#include "encoding.h"
#include "error.h"
#include "file-option.h"
#include "filetype.h"
#include "frame.h"
#include "msg.h"
#include "options.h"
#include "syntax/color.h"
#include "terminal/cursor.h"
#include "terminal/key.h"
#include "terminal/style.h"
#include "util/array.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/str-util.h"
#include "util/unicode.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "view.h"
#include "window.h"

extern char **environ;

typedef enum {
    DTERC = 0x1, // Use "dte" filetype (and syntax highlighter)
    LASTLINE = 0x2, // Move cursor to last line (e.g. most recent history entry)
    MSGLINE = 0x4, // Move cursor to line containing current message
} ShowHandlerFlags;

typedef struct {
    const char name[11];
    uint8_t flags; // ShowHandlerFlags
    String (*dump)(EditorState *e);
    bool (*show)(EditorState *e, const char *name, bool cmdline);
    void (*complete_arg)(EditorState *e, PointerArray *a, const char *prefix);
} ShowHandler;

static void open_temporary_buffer (
    EditorState *e,
    const char *text,
    size_t text_len,
    const char *cmd,
    const char *cmd_arg,
    ShowHandlerFlags flags
) {
    View *view = window_open_new_file(e->window);
    Buffer *buffer = view->buffer;
    buffer->temporary = true;
    do_insert(view, text, text_len);
    set_display_filename(buffer, xasprintf("(%s %s)", cmd, cmd_arg));
    buffer_set_encoding(buffer, encoding_from_type(UTF8), e->options.utf8_bom);

    if (flags & LASTLINE) {
        block_iter_eof(&view->cursor);
        block_iter_prev_line(&view->cursor);
    } else if ((flags & MSGLINE) && e->messages.array.count > 0) {
        block_iter_goto_line(&view->cursor, e->messages.pos);
    }

    if (flags & DTERC) {
        buffer->options.filetype = str_intern("dte");
        set_file_options(e, buffer);
        buffer_update_syntax(e, buffer);
    }
}

static bool show_normal_alias(EditorState *e, const char *alias_name, bool cflag)
{
    const char *cmd_str = find_alias(&e->aliases, alias_name);
    if (!cmd_str) {
        if (find_normal_command(alias_name)) {
            info_msg("%s is a built-in command, not an alias", alias_name);
        } else {
            info_msg("%s is not a known alias", alias_name);
        }
        return true;
    }

    if (cflag) {
        set_input_mode(e, INPUT_COMMAND);
        cmdline_set_text(&e->cmdline, cmd_str);
    } else {
        info_msg("%s is aliased to: %s", alias_name, cmd_str);
    }

    return true;
}

static bool show_binding(EditorState *e, const char *keystr, bool cflag)
{
    KeyCode key;
    if (!parse_key_string(&key, keystr)) {
        return error_msg("invalid key string: %s", keystr);
    }

    // Use canonical key string in printed messages
    char buf[KEYCODE_STR_MAX];
    size_t len = keycode_to_string(key, buf);
    BUG_ON(len == 0);
    keystr = buf;

    if (u_is_unicode(key)) {
        return error_msg("%s is not a bindable key", keystr);
    }

    const CachedCommand *b = lookup_binding(&e->modes[INPUT_NORMAL].key_bindings, key);
    if (!b) {
        info_msg("%s is not bound to a command", keystr);
        return true;
    }

    if (cflag) {
        set_input_mode(e, INPUT_COMMAND);
        cmdline_set_text(&e->cmdline, b->cmd_str);
    } else {
        info_msg("%s is bound to: %s", keystr, b->cmd_str);
    }

    return true;
}

static bool show_color(EditorState *e, const char *color_name, bool cflag)
{
    const TermColor *hl = find_color(&e->colors, color_name);
    if (!hl) {
        info_msg("no color entry with name '%s'", color_name);
        return true;
    }

    if (cflag) {
        CommandLine *c = &e->cmdline;
        set_input_mode(e, INPUT_COMMAND);
        cmdline_clear(c);
        string_append_hl_color(&c->buf, color_name, hl);
        c->pos = c->buf.len;
    } else {
        const char *color_str = term_color_to_string(hl);
        info_msg("color '%s' is set to: %s", color_name, color_str);
    }

    return true;
}

static bool show_cursor(EditorState *e, const char *mode_str, bool cflag)
{
    CursorInputMode mode = cursor_mode_from_str(mode_str);
    if (mode >= NR_CURSOR_MODES) {
        return error_msg("no cursor entry for '%s'", mode_str);
    }

    TermCursorStyle style = e->cursor_styles[mode];
    const char *type = cursor_type_to_str(style.type);
    const char *color = cursor_color_to_str(style.color);
    if (cflag) {
        char buf[64];
        xsnprintf(buf, sizeof buf, "cursor %s %s %s", mode_str, type, color);
        set_input_mode(e, INPUT_COMMAND);
        cmdline_set_text(&e->cmdline, buf);
    } else {
        info_msg("cursor '%s' is set to: %s %s", mode_str, type, color);
    }

    return true;
}

static bool show_env(EditorState *e, const char *name, bool cflag)
{
    const char *value = getenv(name);
    if (!value) {
        info_msg("no environment variable with name '%s'", name);
        return true;
    }

    if (cflag) {
        set_input_mode(e, INPUT_COMMAND);
        cmdline_set_text(&e->cmdline, value);
    } else {
        info_msg("$%s is set to: %s", name, value);
    }

    return true;
}

static String dump_env(EditorState* UNUSED_ARG(e))
{
    String buf = string_new(4096);
    for (size_t i = 0; environ[i]; i++) {
        string_append_cstring(&buf, environ[i]);
        string_append_byte(&buf, '\n');
    }
    return buf;
}

static String dump_setenv(EditorState* UNUSED_ARG(e))
{
    String buf = string_new(4096);
    for (size_t i = 0; environ[i]; i++) {
        const char *str = environ[i];
        const char *delim = strchr(str, '=');
        if (unlikely(!delim || delim == str)) {
            continue;
        }
        string_append_literal(&buf, "setenv ");
        if (unlikely(str[0] == '-' || delim[1] == '-')) {
            string_append_literal(&buf, "-- ");
        }
        const StringView name = string_view(str, delim - str);
        string_append_escaped_arg_sv(&buf, name, true);
        string_append_byte(&buf, ' ');
        string_append_escaped_arg(&buf, delim + 1, true);
        string_append_byte(&buf, '\n');
    }
    return buf;
}

static bool show_builtin(EditorState *e, const char *name, bool cflag)
{
    const BuiltinConfig *cfg = get_builtin_config(name);
    if (!cfg) {
        return error_msg("no built-in config with name '%s'", name);
    }

    const StringView sv = cfg->text;
    if (cflag) {
        buffer_insert_bytes(e->view, sv.data, sv.length);
    } else {
        open_temporary_buffer(e, sv.data, sv.length, "builtin", name, DTERC);
    }

    return true;
}

static bool show_compiler(EditorState *e, const char *name, bool cflag)
{
    const Compiler *compiler = find_compiler(&e->compilers, name);
    if (!compiler) {
        info_msg("no errorfmt entry found for '%s'", name);
        return true;
    }

    String str = string_new(512);
    dump_compiler(compiler, name, &str);
    if (cflag) {
        buffer_insert_bytes(e->view, str.buffer, str.len);
    } else {
        open_temporary_buffer(e, str.buffer, str.len, "errorfmt", name, DTERC);
    }

    string_free(&str);
    return true;
}

static bool show_option(EditorState *e, const char *name, bool cflag)
{
    const char *value = get_option_value_string(e, name);
    if (!value) {
        return error_msg("invalid option name: %s", name);
    }

    if (cflag) {
        set_input_mode(e, INPUT_COMMAND);
        cmdline_set_text(&e->cmdline, value);
    } else {
        info_msg("%s is set to: %s", name, value);
    }

    return true;
}

static void collect_all_options(EditorState* UNUSED_ARG(e), PointerArray *a, const char *prefix)
{
    collect_options(a, prefix, false, false);
}

static void do_collect_cursor_modes(EditorState* UNUSED_ARG(e), PointerArray *a, const char *prefix)
{
    collect_cursor_modes(a, prefix);
}

static void do_collect_builtin_configs(EditorState* UNUSED_ARG(e), PointerArray *a, const char *prefix)
{
    collect_builtin_configs(a, prefix);
}

static void do_collect_builtin_includes(EditorState* UNUSED_ARG(e), PointerArray *a, const char *prefix)
{
    collect_builtin_includes(a, prefix);
}

static bool show_wsplit(EditorState *e, const char *name, bool cflag)
{
    if (!streq(name, "this")) {
        return error_msg("invalid window: %s", name);
    }

    const Window *w = e->window;
    char buf[(4 * DECIMAL_STR_MAX(w->x)) + 4];
    xsnprintf(buf, sizeof buf, "%d,%d %dx%d", w->x, w->y, w->w, w->h);

    if (cflag) {
        set_input_mode(e, INPUT_COMMAND);
        cmdline_set_text(&e->cmdline, buf);
    } else {
        info_msg("current window dimensions: %s", buf);
    }

    return true;
}

static String do_history_dump(const History *history)
{
    const size_t nr_entries = history->entries.count;
    const size_t size = round_size_to_next_multiple(16 * nr_entries, 4096);
    String buf = string_new(size);
    size_t n = 0;
    for (HistoryEntry *e = history->first; e; e = e->next, n++) {
        string_append_cstring(&buf, e->text);
        string_append_byte(&buf, '\n');
    }
    BUG_ON(n != nr_entries);
    return buf;
}

String dump_command_history(EditorState *e)
{
    return do_history_dump(&e->command_history);
}

String dump_search_history(EditorState *e)
{
    return do_history_dump(&e->search_history);
}

typedef struct {
    const char *name;
    const char *value;
} CommandAlias;

static int alias_cmp(const void *ap, const void *bp)
{
    const CommandAlias *a = ap;
    const CommandAlias *b = bp;
    return strcmp(a->name, b->name);
}

String dump_normal_aliases(EditorState *e)
{
    const size_t count = e->aliases.count;
    if (unlikely(count == 0)) {
        return string_new(0);
    }

    // Clone the contents of the HashMap as an array of name/value pairs
    CommandAlias *array = xnew(CommandAlias, count);
    size_t n = 0;
    for (HashMapIter it = hashmap_iter(&e->aliases); hashmap_next(&it); ) {
        array[n++] = (CommandAlias) {
            .name = it.entry->key,
            .value = it.entry->value,
        };
    }

    // Sort the array
    BUG_ON(n != count);
    qsort(array, count, sizeof(array[0]), alias_cmp);

    // Serialize the aliases in sorted order
    String buf = string_new(4096);
    for (size_t i = 0; i < count; i++) {
        const char *name = array[i].name;
        string_append_literal(&buf, "alias ");
        if (unlikely(name[0] == '-')) {
            string_append_literal(&buf, "-- ");
        }
        string_append_escaped_arg(&buf, name, true);
        string_append_byte(&buf, ' ');
        string_append_escaped_arg(&buf, array[i].value, true);
        string_append_byte(&buf, '\n');
    }

    free(array);
    return buf;
}

String dump_all_bindings(EditorState *e)
{
    static const char flags[][4] = {
        [INPUT_NORMAL] = "",
        [INPUT_COMMAND] = "-c ",
        [INPUT_SEARCH] = "-s ",
    };

    static_assert(ARRAYLEN(flags) == ARRAYLEN(e->modes));
    String buf = string_new(4096);
    for (InputMode i = 0, n = ARRAYLEN(e->modes); i < n; i++) {
        const IntMap *bindings = &e->modes[i].key_bindings;
        if (dump_bindings(bindings, flags[i], &buf) && i != n - 1) {
            string_append_byte(&buf, '\n');
        }
    }
    return buf;
}

String dump_frames(EditorState *e)
{
    String str = string_new(4096);
    dump_frame(e->root_frame, 0, &str);
    return str;
}

String dump_compilers(EditorState *e)
{
    String buf = string_new(4096);
    for (HashMapIter it = hashmap_iter(&e->compilers); hashmap_next(&it); ) {
        const char *name = it.entry->key;
        const Compiler *c = it.entry->value;
        dump_compiler(c, name, &buf);
        string_append_byte(&buf, '\n');
    }
    return buf;
}

String dump_cursors(EditorState *e)
{
    String buf = string_new(128);
    for (CursorInputMode m = 0; m < ARRAYLEN(e->cursor_styles); m++) {
        const TermCursorStyle *style = &e->cursor_styles[m];
        string_append_literal(&buf, "cursor ");
        string_append_cstring(&buf, cursor_mode_to_str(m));
        string_append_byte(&buf, ' ');
        string_append_cstring(&buf, cursor_type_to_str(style->type));
        string_append_byte(&buf, ' ');
        string_append_cstring(&buf, cursor_color_to_str(style->color));
        string_append_byte(&buf, '\n');
    }
    return buf;
}

// Dump option values only
String do_dump_options(EditorState *e)
{
    return dump_options(&e->options, &e->buffer->options);
}

// Dump option values and FileOption entries
String dump_options_and_fileopts(EditorState *e)
{
    String str = do_dump_options(e);
    string_append_literal(&str, "\n\n");
    dump_file_options(&e->file_options, &str);
    return str;
}

String do_dump_builtin_configs(EditorState* UNUSED_ARG(e))
{
    return dump_builtin_configs();
}

String do_dump_hl_colors(EditorState *e)
{
    return dump_hl_colors(&e->colors);
}

String do_dump_filetypes(EditorState *e)
{
    return dump_filetypes(&e->filetypes);
}

static String do_dump_messages(EditorState *e)
{
    return dump_messages(&e->messages);
}

static String do_dump_macro(EditorState *e)
{
    return dump_macro(&e->macro);
}

static String do_dump_buffer(EditorState *e)
{
    return dump_buffer(e->buffer);
}

static const ShowHandler show_handlers[] = {
    {"alias", DTERC, dump_normal_aliases, show_normal_alias, collect_normal_aliases},
    {"bind", DTERC, dump_all_bindings, show_binding, collect_bound_normal_keys},
    {"buffer", 0, do_dump_buffer, NULL, NULL},
    {"builtin", 0, do_dump_builtin_configs, show_builtin, do_collect_builtin_configs},
    {"color", DTERC, do_dump_hl_colors, show_color, collect_hl_colors},
    {"command", DTERC | LASTLINE, dump_command_history, NULL, NULL},
    {"cursor", DTERC, dump_cursors, show_cursor, do_collect_cursor_modes},
    {"env", 0, dump_env, show_env, collect_env},
    {"errorfmt", DTERC, dump_compilers, show_compiler, collect_compilers},
    {"ft", DTERC, do_dump_filetypes, NULL, NULL},
    {"hi", DTERC, do_dump_hl_colors, show_color, collect_hl_colors},
    {"include", 0, do_dump_builtin_configs, show_builtin, do_collect_builtin_includes},
    {"macro", DTERC, do_dump_macro, NULL, NULL},
    {"msg", MSGLINE, do_dump_messages, NULL, NULL},
    {"option", DTERC, dump_options_and_fileopts, show_option, collect_all_options},
    {"search", LASTLINE, dump_search_history, NULL, NULL},
    {"set", DTERC, do_dump_options, show_option, collect_all_options},
    {"setenv", DTERC, dump_setenv, show_env, collect_env},
    {"wsplit", 0, dump_frames, show_wsplit, NULL},
};

UNITTEST {
    CHECK_BSEARCH_ARRAY(show_handlers, name, strcmp);
}

bool show(EditorState *e, const char *type, const char *key, bool cflag)
{
    const ShowHandler *handler = BSEARCH(type, show_handlers, vstrcmp);
    if (!handler) {
        return error_msg("invalid argument: '%s'", type);
    }

    if (key) {
        if (!handler->show) {
            return error_msg("'show %s' doesn't take extra arguments", type);
        }
        return handler->show(e, key, cflag);
    }

    String str = handler->dump(e);
    open_temporary_buffer(e, str.buffer, str.len, "show", type, handler->flags);
    string_free(&str);
    return true;
}

void collect_show_subcommands(PointerArray *a, const char *prefix)
{
    COLLECT_STRING_FIELDS(show_handlers, name, a, prefix);
}

void collect_show_subcommand_args(EditorState *e, PointerArray *a, const char *name, const char *arg_prefix)
{
    const ShowHandler *handler = BSEARCH(name, show_handlers, vstrcmp);
    if (handler && handler->complete_arg) {
        handler->complete_arg(e, a, arg_prefix);
    }
}
