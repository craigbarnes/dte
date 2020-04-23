#include <stdlib.h>
#include <sys/types.h>
#include "show.h"
#include "alias.h"
#include "bind.h"
#include "block.h"
#include "buffer.h"
#include "change.h"
#include "cmdline.h"
#include "command.h"
#include "config.h"
#include "editor.h"
#include "encoding/encoding.h"
#include "error.h"
#include "frame.h"
#include "macro.h"
#include "options.h"
#include "syntax/color.h"
#include "terminal/key.h"
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
    char buf[128];
    xsnprintf(buf, sizeof buf, "%d,%d %dx%d", w->x, w->y, w->w, w->h);

    if (cflag) {
        set_input_mode(INPUT_COMMAND);
        cmdline_set_text(&editor.cmdline, buf);
    } else {
        info_msg("current window dimensions: %s", buf);
    }
}

static void show_macro(const char* UNUSED_ARG(name), bool UNUSED_ARG(cflag))
{
    error_msg("'show macro' doesn't take extra arguments");
}

void show(const char *type, const char *key, bool cflag)
{
    static const struct {
        const char name[8];
        void (*show)(const char *name, bool cmdline);
        String (*dump)(void);
    } handlers[] = {
        {"alias", show_alias, dump_aliases},
        {"bind", show_binding, dump_bindings},
        {"color", show_color, dump_hl_colors},
        {"env", show_env, dump_env},
        {"include", show_include, dump_builtin_configs},
        {"macro", show_macro, dump_macro},
        {"option", show_option, dump_options},
        {"wsplit", show_wsplit, dump_frames},
    };

    ssize_t cmdtype = -1;
    for (ssize_t i = 0; i < ARRAY_COUNT(handlers); i++) {
        if (streq(type, handlers[i].name)) {
            cmdtype = i;
            break;
        }
    }
    if (cmdtype < 0) {
        error_msg("invalid argument: '%s'", type);
        return;
    }

    if (key) {
        handlers[cmdtype].show(key, cflag);
        return;
    }

    String s = handlers[cmdtype].dump();
    View *v = window_open_new_file(window);
    v->buffer->temporary = true;
    do_insert(s.buffer, s.len);
    string_free(&s);
    set_display_filename(v->buffer, xasprintf("(show %s)", type));
    v->buffer->encoding = encoding_from_type(UTF8);
    if (handlers[cmdtype].show != show_wsplit) {
        v->buffer->options.filetype = str_intern("dte");
        buffer_update_syntax(v->buffer);
    }
}
