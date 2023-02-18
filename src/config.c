#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "config.h"
#include "commands.h"
#include "editor.h"
#include "error.h"
#include "syntax/color.h"
#include "util/debug.h"
#include "util/readfile.h"
#include "util/str-util.h"
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

void exec_config(CommandRunner *runner, StringView config)
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
            handle_command(runner, string_borrow_cstring(&buf));
            string_clear(&buf);
        }
    }

    if (unlikely(buf.len)) {
        // This can only happen if the last line had a line continuation
        handle_command(runner, string_borrow_cstring(&buf));
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

int do_read_config(CommandRunner *runner, const char *filename, ConfigFlags flags)
{
    const bool must_exist = flags & CFG_MUST_EXIST;
    const bool builtin = flags & CFG_BUILTIN;

    if (builtin) {
        const BuiltinConfig *cfg = get_builtin_config(filename);
        int err = 0;
        if (cfg) {
            current_config.file = filename;
            current_config.line = 1;
            exec_config(runner, cfg->text);
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
    exec_config(runner, string_view(buf, size));
    free(buf);
    return 0;
}

int read_config(CommandRunner *runner, const char *filename, ConfigFlags flags)
{
    // Recursive
    const ConfigState saved = current_config;
    int ret = do_read_config(runner, filename, flags);
    current_config = saved;
    return ret;
}

void exec_builtin_color_reset(EditorState *e)
{
    clear_hl_colors(&e->colors);
    const StringView reset = string_view(builtin_color_reset, sizeof(builtin_color_reset) - 1);
    const ConfigState saved = current_config;
    current_config.file = "color/reset";
    current_config.line = 1;
    exec_normal_config(e, reset);
    current_config = saved;
}

void exec_builtin_rc(EditorState *e)
{
    exec_builtin_color_reset(e);
    const StringView rc = string_view(builtin_rc, sizeof(builtin_rc) - 1);
    const ConfigState saved = current_config;
    current_config.file = "rc";
    current_config.line = 1;
    exec_normal_config(e, rc);
    current_config = saved;
}

void collect_builtin_configs(PointerArray *a, const char *prefix)
{
    for (size_t i = 0; i < ARRAYLEN(builtin_configs); i++) {
        const char *name = builtin_configs[i].name;
        if (str_has_prefix(name, prefix)) {
            ptr_array_append(a, xstrdup(name));
        }
    }
}

void collect_builtin_includes(PointerArray *a, const char *prefix)
{
    for (size_t i = 0; i < ARRAYLEN(builtin_configs); i++) {
        const char *name = builtin_configs[i].name;
        if (str_has_prefix(name, prefix) && !str_has_prefix(name, "syntax/")) {
            ptr_array_append(a, xstrdup(name));
        }
    }
}

UNITTEST {
    BUG_ON(!get_builtin_config("rc"));
    BUG_ON(!get_builtin_config("color/reset"));
}
