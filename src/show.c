#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "show.h"
#include "alias.h"
#include "bind.h"
#include "block.h"
#include "buffer.h"
#include "change.h"
#include "cmdline.h"
#include "command/env.h"
#include "command/macro.h"
#include "commands.h"
#include "completion.h"
#include "config.h"
#include "editor.h"
#include "encoding.h"
#include "error.h"
#include "file-option.h"
#include "frame.h"
#include "options.h"
#include "syntax/color.h"
#include "terminal/color.h"
#include "terminal/key.h"
#include "util/bsearch.h"
#include "util/hashset.h"
#include "util/str-util.h"
#include "util/string.h"
#include "util/unicode.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "view.h"
#include "window.h"

static void show_alias(const char *alias_name, bool cflag)
{
    const char *cmd_str = find_alias(alias_name);
    if (!cmd_str) {
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
    if (!b) {
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
    const TermColor *hl = find_color(color_name);
    if (!hl) {
        error_msg("no color entry with name '%s'", color_name);
        return;
    }

    const char *color_str = term_color_to_string(hl);
    if (cflag) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, color_str);
    } else {
        info_msg("color '%s' is set to: %s", color_name, color_str);
    }
}

static void show_env(const char *name, bool cflag)
{
    const char *value = getenv(name);
    if (!value) {
        error_msg("no environment variable with name '%s'", name);
        return;
    }

    if (cflag) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, value);
    } else {
        info_msg("$%s is set to: %s", name, value);
    }
}

static String dump_env(void)
{
    extern char **environ;
    String buf = string_new(4096);
    for (size_t i = 0; environ[i]; i++) {
        string_append_cstring(&buf, environ[i]);
        string_append_byte(&buf, '\n');
    }
    return buf;
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
    char buf[(4 * DECIMAL_STR_MAX(w->x)) + 4];
    xsnprintf(buf, sizeof buf, "%d,%d %dx%d", w->x, w->y, w->w, w->h);

    if (cflag) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, buf);
    } else {
        info_msg("current window dimensions: %s", buf);
    }
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

static String dump_command_history(void)
{
    return do_history_dump(&editor.command_history);
}

static String dump_search_history(void)
{
    return do_history_dump(&editor.search_history);
}

typedef struct {
    const char name[8];
    void (*show)(const char *name, bool cmdline);
    String (*dump)(void);
    void (*complete_arg)(const char *prefix);
    bool dumps_dterc_syntax;
} ShowHandler;

static const ShowHandler handlers[] = {
    {"alias", show_alias, dump_aliases, collect_aliases, true},
    {"bind", show_binding, dump_bindings, collect_bound_keys, true},
    {"color", show_color, dump_hl_colors, collect_hl_colors, true},
    {"command", NULL, dump_command_history, NULL, true},
    {"env", show_env, dump_env, collect_env, false},
    {"include", show_include, dump_builtin_configs, collect_builtin_configs, false},
    {"macro", NULL, dump_macro, NULL, true},
    {"option", show_option, dump_options, collect_options, true},
    {"search", NULL, dump_search_history, NULL, false},
    {"wsplit", show_wsplit, dump_frames, NULL, false},
};

UNITTEST {
    CHECK_BSEARCH_ARRAY(handlers, name, strcmp);
}

void show(const char *type, const char *key, bool cflag)
{
    const ShowHandler *handler = BSEARCH(type, handlers, (CompareFunction)strcmp);
    if (!handler) {
        error_msg("invalid argument: '%s'", type);
        return;
    }

    if (key) {
        if (handler->show) {
            handler->show(key, cflag);
        } else {
            error_msg("'show %s' doesn't take extra arguments", type);
        }
        return;
    }

    String s = handler->dump();
    View *v = window_open_new_file(window);
    v->buffer->temporary = true;
    do_insert(s.buffer, s.len);
    string_free(&s);
    set_display_filename(v->buffer, xasprintf("(show %s)", type));
    buffer_set_encoding(v->buffer, encoding_from_type(UTF8));
    if (handler->dumps_dterc_syntax) {
        v->buffer->options.filetype = str_intern("dte");
        set_file_options(v->buffer);
        buffer_update_syntax(v->buffer);
    }
}

void collect_show_subcommands(const char *prefix)
{
    for (size_t i = 0; i < ARRAY_COUNT(handlers); i++) {
        if (str_has_prefix(handlers[i].name, prefix)) {
            add_completion(xstrdup(handlers[i].name));
        }
    }
}

void collect_show_subcommand_args(const char *name, const char *arg_prefix)
{
    const ShowHandler *handler = BSEARCH(name, handlers, (CompareFunction)strcmp);
    if (handler && handler->complete_arg) {
        handler->complete_arg(arg_prefix);
    }
}
