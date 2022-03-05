#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include "config.h"
#include "commands.h"
#include "editor.h"
#include "error.h"
#include "syntax/color.h"
#include "terminal/terminal.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/string.h"
#include "util/xmalloc.h"
#include "../build/builtin-config.h"

ConfigState current_config;

// Odd number of backslashes at end of line?
static bool has_line_continuation(StringView line)
{
    ssize_t pos = line.length - 1;
    while (pos >= 0 && line.data[pos] == '\\') {
        pos--;
    }
    return (line.length - 1 - pos) & 1;
}

UNITTEST {
    BUG_ON(has_line_continuation(string_view(NULL, 0)));
    BUG_ON(has_line_continuation(strview_from_cstring("0")));
    BUG_ON(!has_line_continuation(strview_from_cstring("1 \\")));
    BUG_ON(has_line_continuation(strview_from_cstring("2 \\\\")));
    BUG_ON(!has_line_continuation(strview_from_cstring("3 \\\\\\")));
    BUG_ON(has_line_continuation(strview_from_cstring("4 \\\\\\\\")));
}

void exec_config(const CommandSet *cmds, StringView config)
{
    String buf = string_new(1024);

    for (size_t i = 0, n = config.length; i < n; current_config.line++) {
        StringView line = buf_slice_next_line(config.data, &i, n);
        strview_trim_left(&line);
        if (buf.len == 0 && strview_has_prefix(&line, "#")) {
            // Comment line
            continue;
        }
        if (has_line_continuation(line)) {
            line.length--;
            string_append_strview(&buf, &line);
        } else {
            string_append_strview(&buf, &line);
            handle_command(cmds, string_borrow_cstring(&buf), false);
            string_clear(&buf);
        }
    }

    if (unlikely(buf.len)) {
        // This can only happen if the last line had a line continuation
        handle_command(cmds, string_borrow_cstring(&buf), false);
    }

    string_free(&buf);
}

String dump_builtin_configs(void)
{
    String str = string_new(1024);
    for (size_t i = 0; i < ARRAYLEN(builtin_configs); i++) {
        string_append_cstring(&str, builtin_configs[i].name);
        string_append_byte(&str, '\n');
    }
    return str;
}

void collect_builtin_configs(PointerArray *a, const char *prefix)
{
    for (size_t i = 0; i < ARRAYLEN(builtin_configs); i++) {
        const BuiltinConfig *cfg = &builtin_configs[i];
        if (str_has_prefix(cfg->name, "syntax/")) {
            return;
        } else if (str_has_prefix(cfg->name, prefix)) {
            ptr_array_append(a, xstrdup(cfg->name));
        }
    }
}

const BuiltinConfig *get_builtin_config(const char *name)
{
    for (size_t i = 0; i < ARRAYLEN(builtin_configs); i++) {
        if (streq(name, builtin_configs[i].name)) {
            return &builtin_configs[i];
        }
    }
    return NULL;
}

const BuiltinConfig *get_builtin_configs_array(size_t *nconfigs)
{
    *nconfigs = ARRAYLEN(builtin_configs);
    return &builtin_configs[0];
}

int do_read_config(const CommandSet *cmds, const char *filename, ConfigFlags flags)
{
    const bool must_exist = flags & CFG_MUST_EXIST;
    const bool builtin = flags & CFG_BUILTIN;

    if (builtin) {
        const BuiltinConfig *cfg = get_builtin_config(filename);
        int err = 0;
        if (cfg) {
            current_config.file = filename;
            current_config.line = 1;
            exec_config(cmds, cfg->text);
        } else if (must_exist) {
            error_msg (
                "Error reading '%s': no built-in config exists for that path",
                filename
            );
            err = 1;
        }
        return err;
    }

    char *buf;
    ssize_t size = read_file(filename, &buf);
    if (size < 0) {
        int err = errno;
        if (err != ENOENT || must_exist) {
            error_msg("Error reading %s: %s", filename, strerror(err));
        }
        return err;
    }

    current_config.file = filename;
    current_config.line = 1;
    exec_config(cmds, string_view(buf, size));
    free(buf);
    return 0;
}

int read_config(const CommandSet *cmds, const char *filename, ConfigFlags flags)
{
    // Recursive
    const ConfigState saved = current_config;
    int ret = do_read_config(cmds, filename, flags);
    current_config = saved;
    return ret;
}

void exec_builtin_color_reset(void)
{
    clear_hl_colors(&editor.colors);
    bool colors = terminal.color_type >= TERM_8_COLOR;
    const char *cfg = colors ? "color/reset" : "color/reset-basic";
    read_config(&normal_commands, cfg, CFG_MUST_EXIST | CFG_BUILTIN);
}

void exec_builtin_rc(void)
{
    exec_builtin_color_reset();
    read_config(&normal_commands, "rc", CFG_MUST_EXIST | CFG_BUILTIN);
}

UNITTEST {
    // Built-in configs can be customized, but these 3 are required:
    BUG_ON(!get_builtin_config("rc"));
    BUG_ON(!get_builtin_config("color/reset"));
    BUG_ON(!get_builtin_config("color/reset-basic"));
}
