#include <stdlib.h>
#include <string.h>
#include "show.h"
#include "bind.h"
#include "buffer.h"
#include "change.h"
#include "cmdline.h"
#include "command/alias.h"
#include "command/error.h"
#include "command/macro.h"
#include "command/serialize.h"
#include "commands.h"
#include "compiler.h"
#include "completion.h"
#include "config.h"
#include "edit.h"
#include "file-option.h"
#include "filetype.h"
#include "frame.h"
#include "mode.h"
#include "msg.h"
#include "options.h"
#include "syntax/color.h"
#include "tag.h"
#include "terminal/cursor.h"
#include "terminal/key.h"
#include "terminal/style.h"
#include "util/array.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/environ.h"
#include "util/intern.h"
#include "util/log.h"
#include "util/unicode.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "util/xstring.h"
#include "view.h"
#include "window.h"

typedef enum {
    DTERC = 1 << 0, // Use "dte" filetype (and syntax highlighter)
    LASTLINE = 1 << 1, // Move cursor to last line (e.g. most recent history entry)
    MSGLINE = 1 << 2, // Move cursor to line corresponding to `e->messages[0].pos`
} ShowHandlerFlags;

typedef struct {
    const char name[10];
    uint8_t flags; // ShowHandlerFlags
    DumpFunc dump;
    bool (*show)(EditorState *e, const char *name, bool cmdline);
    void (*complete_arg)(EditorState *e, PointerArray *a, const char *prefix);
} ShowHandler;

// NOLINTNEXTLINE(readability-function-size)
static void open_temporary_buffer (
    EditorState *e,
    const char *text,
    size_t text_len,
    const char *cmd,
    const char *cmd_arg,
    const char *filetype,
    ShowHandlerFlags flags
) {
    const char *sp = cmd_arg[0] ? " " : "";
    View *view = window_open_new_file(e->window);
    Buffer *buffer = view->buffer;
    buffer_set_display_filename(buffer, xasprintf("(%s%s%s)", cmd, sp, cmd_arg));
    buffer->temporary = true;

    filetype = filetype ? filetype : ((flags & DTERC) ? "dte" : NULL);
    if (filetype) {
        buffer->options.filetype = str_intern(filetype);
        set_file_options(e, buffer);
        buffer_update_syntax(e, buffer);
    }

    if (text_len == 0) {
        return;
    }

    BUG_ON(!text);

    // We don't use buffer_insert_bytes() here, because the call to
    // record_insert() would make the initial text contents undoable
    if (unlikely(text[text_len - 1] != '\n')) {
        LOG_ERROR("no final newline in text");
        do_insert(view, "\n", 1);
    }
    do_insert(view, text, text_len);

    if (flags & LASTLINE) {
        block_iter_eof(&view->cursor);
        block_iter_prev_line(&view->cursor);
    } else if (flags & MSGLINE) {
        const MessageList *msgs = &e->messages[0];
        if (msgs->array.count > 0) {
            block_iter_goto_line(&view->cursor, msgs->pos);
        }
    }
}

static bool show_normal_alias(EditorState *e, const char *alias_name, bool cflag)
{
    const char *cmd_str = find_alias(&e->aliases, alias_name);
    if (!cmd_str) {
        if (find_normal_command(alias_name)) {
            return info_msg(&e->err, "%s is a built-in command, not an alias", alias_name);
        }
        return info_msg(&e->err, "%s is not a known alias", alias_name);
    }

    if (!cflag) {
        return info_msg(&e->err, "%s is aliased to: %s", alias_name, cmd_str);
    }

    push_input_mode(e, e->command_mode);
    cmdline_set_text(&e->cmdline, cmd_str);
    return true;
}

static bool show_binding(EditorState *e, const char *keystr, bool cflag)
{
    KeyCode key = keycode_from_str(keystr);
    if (key == KEY_NONE) {
        return error_msg(&e->err, "invalid key string: %s", keystr);
    }

    // Use canonical key string in printed messages
    char buf[KEYCODE_STR_BUFSIZE];
    size_t len = keycode_to_str(key, buf);
    BUG_ON(len == 0);
    keystr = buf;

    if (u_is_unicode(key)) {
        return error_msg(&e->err, "%s is not a bindable key", keystr);
    }

    const CachedCommand *b = lookup_binding(&e->normal_mode->key_bindings, key);
    if (!b) {
        return info_msg(&e->err, "%s is not bound to a command", keystr);
    }

    if (!cflag) {
        return info_msg(&e->err, "%s is bound to: %s", keystr, b->cmd_str);
    }

    push_input_mode(e, e->command_mode);
    cmdline_set_text(&e->cmdline, b->cmd_str);
    return true;
}

static bool show_color(EditorState *e, const char *name, bool cflag)
{
    const TermStyle *hl = find_style(&e->styles, name);
    if (!hl) {
        return info_msg(&e->err, "no color entry with name '%s'", name);
    }

    if (!cflag) {
        char buf[TERM_STYLE_BUFSIZE];
        const char *style_str = term_style_to_string(buf, hl);
        return info_msg(&e->err, "color '%s' is set to: %s", name, style_str);
    }

    CommandLine *c = &e->cmdline;
    push_input_mode(e, e->command_mode);
    cmdline_clear(c);
    string_append_hl_style(&c->buf, name, hl);
    c->pos = c->buf.len;
    return true;
}

static bool show_cursor(EditorState *e, const char *mode_str, bool cflag)
{
    CursorInputMode mode = cursor_mode_from_str(mode_str);
    if (mode >= NR_CURSOR_MODES) {
        return error_msg(&e->err, "no cursor entry for '%s'", mode_str);
    }

    TermCursorStyle style = e->cursor_styles[mode];
    char colorbuf[COLOR_STR_BUFSIZE];
    const char *type = cursor_type_to_str(style.type);
    const char *color = cursor_color_to_str(colorbuf, style.color);

    if (!cflag) {
        return info_msg(&e->err, "cursor '%s' is set to: %s %s", mode_str, type, color);
    }

    char buf[64];
    xsnprintf(buf, sizeof buf, "cursor %s %s %s", mode_str, type, color);
    push_input_mode(e, e->command_mode);
    cmdline_set_text(&e->cmdline, buf);
    return true;
}

static bool show_env(EditorState *e, const char *name, bool cflag)
{
    const char *value = getenv(name);
    if (!value) {
        return info_msg(&e->err, "no environment variable with name '%s'", name);
    }

    if (!cflag) {
        return info_msg(&e->err, "$%s is set to: %s", name, value);
    }

    push_input_mode(e, e->command_mode);
    cmdline_set_text(&e->cmdline, value);
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

static const char *flag_for_builtin_mode(const EditorState *e, const ModeHandler *m)
{
    if (m == e->normal_mode) {
        return "";
    } else if (m == e->command_mode) {
        return "-c ";
    } else if (m == e->search_mode) {
        return "-s ";
    }
    return NULL;
}

static bool show_mode(EditorState *e, const char *name, bool cflag)
{
    const ModeHandler *mode = get_mode_handler(&e->modes, name);
    if (!mode) {
        return info_msg(&e->err, "no mode with name '%s'", name);
    }

    String str = string_new(4096);
    const char *flag = flag_for_builtin_mode(e, mode);
    if (!flag) {
        string_append_def_mode(&str, mode);
        string_append_byte(&str, '\n');
    }

    dump_bindings(&mode->key_bindings, flag ? flag : name, &str);

    if (cflag) {
        buffer_insert_bytes(e->view, str.buffer, str.len);
    } else {
        open_temporary_buffer(e, str.buffer, str.len, "def-mode", name, NULL, DTERC);
    }

    string_free(&str);
    return true;
}

static bool show_builtin(EditorState *e, const char *name, bool cflag)
{
    const BuiltinConfig *cfg = get_builtin_config(name);
    if (!cfg) {
        return error_msg(&e->err, "no built-in config with name '%s'", name);
    }

    const StringView sv = cfg->text;
    if (cflag) {
        buffer_insert_bytes(e->view, sv.data, sv.length);
    } else {
        bool script = str_has_prefix(name, "script/");
        const char *ft = script ? filetype_str_from_extension(name) : "dte";
        open_temporary_buffer(e, sv.data, sv.length, "builtin", name, ft, 0);
    }

    return true;
}

static bool show_compiler(EditorState *e, const char *name, bool cflag)
{
    const Compiler *compiler = find_compiler(&e->compilers, name);
    if (!compiler) {
        return info_msg(&e->err, "no errorfmt entry found for '%s'", name);
    }

    String str = string_new(512);
    dump_compiler(compiler, name, &str);
    if (cflag) {
        buffer_insert_bytes(e->view, str.buffer, str.len);
    } else {
        open_temporary_buffer(e, str.buffer, str.len, "errorfmt", name, NULL, DTERC);
    }

    string_free(&str);
    return true;
}

static bool show_msg(EditorState *e, const char *name, bool cflag)
{
    size_t idx = name[0] - 'A';
    if (idx >= ARRAYLEN(e->messages) || name[1] != '\0') {
        return info_msg(&e->err, "no message list '%s'", name);
    }

    const MessageList *msgs = &e->messages[idx];
    String str = dump_messages(msgs);

    if (cflag) {
        buffer_insert_bytes(e->view, str.buffer, str.len);
    } else {
        open_temporary_buffer(e, str.buffer, str.len, "msg", name, NULL, 0);
        if (msgs->array.count > 0) {
            block_iter_goto_line(&e->view->cursor, msgs->pos);
        }
    }

    string_free(&str);
    return true;
}

static bool show_option(EditorState *e, const char *name, bool cflag)
{
    const char *value = get_option_value_string(e, name);
    if (!value) {
        return error_msg(&e->err, "invalid option name: %s", name);
    }

    if (!cflag) {
        return info_msg(&e->err, "%s is set to: %s", name, value);
    }

    push_input_mode(e, e->command_mode);
    cmdline_set_text(&e->cmdline, value);
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

static void do_collect_modes(EditorState *e, PointerArray *a, const char *prefix)
{
    collect_modes(&e->modes, a, prefix);
}

static void do_collect_env(EditorState* UNUSED_ARG(e), PointerArray *a, const char *prefix)
{
    collect_env(environ, a, strview_from_cstring(prefix), "");
}

static void collect_show_msg_args(EditorState* UNUSED_ARG(e), PointerArray *a, const char *prefix)
{
    static const char args[][2] = {"A", "B", "C"};
    COLLECT_STRINGS(args, a, prefix);
}

static bool show_wsplit(EditorState *e, const char *name, bool cflag)
{
    if (!streq(name, "this")) {
        return error_msg(&e->err, "invalid window: %s", name);
    }

    const Window *w = e->window;
    char buf[(4 * DECIMAL_STR_MAX(w->x)) + 4];
    xsnprintf(buf, sizeof buf, "%d,%d %dx%d", w->x, w->y, w->w, w->h);

    if (!cflag) {
        return info_msg(&e->err, "current window dimensions: %s", buf);
    }

    push_input_mode(e, e->command_mode);
    cmdline_set_text(&e->cmdline, buf);
    return true;
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

static String dump_normal_aliases(EditorState *e)
{
    const size_t count = e->aliases.count;
    if (unlikely(count == 0)) {
        return string_new(0);
    }

    // Clone the contents of the HashMap as an array of name/value pairs
    CommandAlias *array = xmallocarray(count, sizeof(*array));
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

typedef struct {
    const char *name;
    const ModeHandler *handler;
} ModeHandlerEntry;

static int mhe_cmp(const void *ap, const void *bp)
{
    const ModeHandlerEntry *a = ap;
    const ModeHandlerEntry *b = bp;
    return strcmp(a->name, b->name);
}

static String dump_all_bindings(EditorState *e)
{
    String buf = string_new(4096);
    if (dump_bindings(&e->normal_mode->key_bindings, "", &buf)) {
        string_append_byte(&buf, '\n');
    }

    size_t count = e->modes.count;
    BUG_ON(count < 3);
    count -= 3;

    if (count) {
        // Clone custom modes in HashMap as an array
        ModeHandlerEntry *array = xmallocarray(count, sizeof(*array));
        size_t n = 0;
        for (HashMapIter it = hashmap_iter(&e->modes); hashmap_next(&it); ) {
            const char *name = it.entry->key;
            const ModeHandler *handler = it.entry->value;
            bool is_builtin_mode = !!flag_for_builtin_mode(e, handler);
            if (is_builtin_mode) {
                continue;
            }
            array[n++] = (ModeHandlerEntry) {
                .name = name,
                .handler = handler,
            };
        }

        // Sort the array
        BUG_ON(n != count);
        qsort(array, count, sizeof(array[0]), mhe_cmp);

        // Serialize bindings for each mode, sorted by mode name
        for (size_t i = 0; i < count; i++) {
            const char *name = array[i].name;
            const ModeHandler *handler = array[i].handler;
            const IntMap *bindings = &handler->key_bindings;
            if (dump_bindings(bindings, name, &buf)) {
                string_append_byte(&buf, '\n');
            }
        }

        free(array);
    }

    dump_bindings(&e->command_mode->key_bindings, "-c ", &buf);
    string_append_byte(&buf, '\n');
    dump_bindings(&e->search_mode->key_bindings, "-s ", &buf);
    return buf;
}

static String dump_modes(EditorState *e)
{
    // TODO: Serialize in alphabetical order, instead of table order?
    String buf = string_new(256);
    for (HashMapIter it = hashmap_iter(&e->modes); hashmap_next(&it); ) {
        const ModeHandler *mode = it.entry->value;
        string_append_def_mode(&buf, mode);
        string_append_byte(&buf, '\n');
    }
    return buf;
}

static String dump_frames(EditorState *e)
{
    String str = string_new(4096);
    dump_frame(e->root_frame, 0, &str);
    return str;
}

static String dump_compilers(EditorState *e)
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

static String dump_cursors(EditorState *e)
{
    String buf = string_new(128);
    char colorbuf[COLOR_STR_BUFSIZE];
    for (CursorInputMode m = 0; m < ARRAYLEN(e->cursor_styles); m++) {
        const TermCursorStyle *style = &e->cursor_styles[m];
        string_append_literal(&buf, "cursor ");
        string_append_cstring(&buf, cursor_mode_to_str(m));
        string_append_byte(&buf, ' ');
        string_append_cstring(&buf, cursor_type_to_str(style->type));
        string_append_byte(&buf, ' ');
        string_append_cstring(&buf, cursor_color_to_str(colorbuf, style->color));
        string_append_byte(&buf, '\n');
    }
    return buf;
}

// Dump option values and FileOption entries
static String dump_options_and_fileopts(EditorState *e)
{
    String str = dump_options(&e->options, &e->buffer->options);
    string_append_literal(&str, "\n\n");
    dump_file_options(&e->file_options, &str);
    return str;
}

static String dump_paste(EditorState *e)
{
    const Clipboard *clip = &e->clipboard;
    return string_new_from_buf(clip->buf, clip->len);
}

static String do_dump_options(EditorState *e) {return dump_options(&e->options, &e->buffer->options);}
static String do_dump_builtin_configs(EditorState* UNUSED_ARG(e)) {return dump_builtin_configs();}
static String do_dump_hl_styles(EditorState *e) {return dump_hl_styles(&e->styles);}
static String do_dump_filetypes(EditorState *e) {return dump_filetypes(&e->filetypes);}
static String do_dump_messages_a(EditorState *e) {return dump_messages(&e->messages[0]);}
static String do_dump_macro(EditorState *e) {return dump_macro(&e->macro);}
static String do_dump_buffer(EditorState *e) {return dump_buffer(e->view);}
static String do_dump_tags(EditorState *e) {return dump_tags(&e->tagfile, &e->err);}
static String dump_command_history(EditorState *e) {return history_dump(&e->command_history);}
static String dump_search_history(EditorState *e) {return history_dump(&e->search_history);}
static String dump_file_history(EditorState *e) {return file_history_dump(&e->file_history);}
static String dump_show_subcmds(EditorState *e); // Forward declaration

static const ShowHandler show_handlers[] = {
    {"alias", DTERC, dump_normal_aliases, show_normal_alias, collect_normal_aliases},
    {"bind", DTERC, dump_all_bindings, show_binding, collect_bound_normal_keys},
    {"buffer", 0, do_dump_buffer, NULL, NULL},
    {"builtin", 0, do_dump_builtin_configs, show_builtin, do_collect_builtin_configs},
    {"color", DTERC, do_dump_hl_styles, show_color, collect_hl_styles},
    {"command", DTERC | LASTLINE, dump_command_history, NULL, NULL},
    {"cursor", DTERC, dump_cursors, show_cursor, do_collect_cursor_modes},
    {"def-mode", DTERC, dump_modes, show_mode, do_collect_modes},
    {"env", 0, dump_env, show_env, do_collect_env},
    {"errorfmt", DTERC, dump_compilers, show_compiler, collect_compilers},
    {"ft", DTERC, do_dump_filetypes, NULL, NULL},
    {"hi", DTERC, do_dump_hl_styles, show_color, collect_hl_styles},
    {"include", 0, do_dump_builtin_configs, show_builtin, do_collect_builtin_includes},
    {"macro", DTERC, do_dump_macro, NULL, NULL},
    {"msg", MSGLINE, do_dump_messages_a, show_msg, collect_show_msg_args},
    {"open", LASTLINE, dump_file_history, NULL, NULL},
    {"option", DTERC, dump_options_and_fileopts, show_option, collect_all_options},
    {"paste", 0, dump_paste, NULL, NULL},
    {"search", LASTLINE, dump_search_history, NULL, NULL},
    {"set", DTERC, do_dump_options, show_option, collect_all_options},
    {"setenv", DTERC, dump_setenv, show_env, do_collect_env},
    {"show", DTERC, dump_show_subcmds, NULL, NULL},
    {"tag", 0, do_dump_tags, NULL, NULL},
    {"wsplit", 0, dump_frames, show_wsplit, NULL},
};

static const char *arg_hint_for_subcmd(const ShowHandler *handler)
{
    const void *show = handler->show;
    return (show == show_binding) ? " [key]" : (show ? " [name]" : "");
}

static String dump_show_subcmds(EditorState* UNUSED_ARG(e))
{
    String str = string_new(256);
    string_append_literal(&str, "# Available `show` sub-commands:\n");
    for (size_t i = 0; i < ARRAYLEN(show_handlers); i++) {
        const ShowHandler *handler = &show_handlers[i];
        string_append_cstring(&str, "show ");
        string_append_cstring(&str, handler->name);
        string_append_cstring(&str, arg_hint_for_subcmd(handler));
        string_append_byte(&str, '\n');
    }
    return str;
}

static const ShowHandler *lookup_show_handler(const char *name)
{
    const ShowHandler *handler = BSEARCH(name, show_handlers, vstrcmp);
    return handler;
}

DumpFunc get_dump_function(const char *name)
{
    const ShowHandler *handler = lookup_show_handler(name);
    return handler ? handler->dump : NULL;
}

UNITTEST {
    // NOLINTBEGIN(bugprone-assert-side-effect)
    CHECK_BSEARCH_ARRAY(show_handlers, name);
    BUG_ON(!lookup_show_handler("alias"));
    BUG_ON(!lookup_show_handler("set"));
    BUG_ON(!lookup_show_handler("show"));
    BUG_ON(!lookup_show_handler("wsplit"));
    BUG_ON(lookup_show_handler("alia"));
    BUG_ON(lookup_show_handler("sete"));
    BUG_ON(lookup_show_handler(""));
    BUG_ON(!get_dump_function("msg"));
    BUG_ON(get_dump_function("_"));
    // NOLINTEND(bugprone-assert-side-effect)
}

bool show(EditorState *e, const char *type, const char *key, bool cflag)
{
    const ShowHandler *handler = lookup_show_handler(type ? type : "show");
    if (!handler) {
        return error_msg(&e->err, "invalid argument: '%s'", type);
    }

    if (!type) {
        type = "";
    }

    if (key) {
        if (!handler->show) {
            return error_msg(&e->err, "'show %s' doesn't take extra arguments", type);
        }
        return handler->show(e, key, cflag);
    }

    String str = handler->dump(e);
    open_temporary_buffer(e, str.buffer, str.len, "show", type, NULL, handler->flags);
    string_free(&str);
    return true;
}

void collect_show_subcommands(PointerArray *a, const char *prefix)
{
    COLLECT_STRING_FIELDS(show_handlers, name, a, prefix);
}

void collect_show_subcommand_args(EditorState *e, PointerArray *a, const char *name, const char *arg_prefix)
{
    const ShowHandler *handler = lookup_show_handler(name);
    if (handler && handler->complete_arg) {
        handler->complete_arg(e, a, arg_prefix);
    }
}
