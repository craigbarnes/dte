#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "config.h"
#include "builtin-config.h"
#include "commands.h"
#include "editor.h"
#include "error.h"
#include "syntax/color.h"
#include "util/debug.h"
#include "util/readfile.h"
#include "util/str-util.h"

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
    // NOLINTBEGIN(bugprone-assert-side-effect)
    BUG_ON(has_line_continuation(string_view(NULL, 0)));
    BUG_ON(has_line_continuation(strview_from_cstring("0")));
    BUG_ON(!has_line_continuation(strview_from_cstring("1 \\")));
    BUG_ON(has_line_continuation(strview_from_cstring("2 \\\\")));
    BUG_ON(!has_line_continuation(strview_from_cstring("3 \\\\\\")));
    BUG_ON(has_line_continuation(strview_from_cstring("4 \\\\\\\\")));
    // NOLINTEND(bugprone-assert-side-effect)
}

void exec_config(CommandRunner *runner, StringView config)
{
    String buf = string_new(1024);

    for (size_t i = 0, n = config.length; i < n; runner->ebuf->config_line++) {
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
    ErrorBuffer *ebuf = runner->ebuf;
    bool must_exist = flags & CFG_MUST_EXIST;

    if (flags & CFG_BUILTIN) {
        const BuiltinConfig *cfg = get_builtin_config(filename);
        int err = 0;
        if (cfg) {
            ebuf->config_filename = filename;
            ebuf->config_line = 1;
            exec_config(runner, cfg->text);
        } else if (must_exist) {
            error_msg (
                ebuf,
                "Error reading '%s': no built-in config exists for that path",
                filename
            );
            err = 1;
        }
        return err;
    }

    char *buf;
    ssize_t size = read_file(filename, &buf, 0);
    if (size < 0) {
        int err = errno;
        if (err != ENOENT || must_exist) {
            error_msg(ebuf, "Error reading %s: %s", filename, strerror(err));
        }
        return err;
    }

    ebuf->config_filename = filename;
    ebuf->config_line = 1;
    exec_config(runner, string_view(buf, size));
    free(buf);
    return 0;
}

int read_config(CommandRunner *runner, const char *filename, ConfigFlags flags)
{
    // Recursive
    ErrorBuffer *ebuf = runner->ebuf;
    const char *saved_file = ebuf->config_filename;
    const unsigned int saved_line = ebuf->config_line;
    int ret = do_read_config(runner, filename, flags);
    ebuf->config_filename = saved_file;
    ebuf->config_line = saved_line;
    return ret;
}

static void exec_builtin_config(EditorState *e, StringView cfg, const char *name)
{
    ErrorBuffer *ebuf = &e->err;
    const char *saved_file = ebuf->config_filename;
    const unsigned int saved_line = ebuf->config_line;
    ebuf->config_filename = name;
    ebuf->config_line = 1;
    exec_normal_config(e, cfg);
    ebuf->config_filename = saved_file;
    ebuf->config_line = saved_line;
}

void exec_builtin_color_reset(EditorState *e)
{
    clear_hl_styles(&e->styles);
    StringView reset = string_view(builtin_color_reset, sizeof(builtin_color_reset) - 1);
    exec_builtin_config(e, reset, "color/reset");
}

void exec_builtin_rc(EditorState *e)
{
    exec_builtin_color_reset(e);
    StringView rc = string_view(builtin_rc, sizeof(builtin_rc) - 1);
    exec_builtin_config(e, rc, "rc");
}

void collect_builtin_configs(PointerArray *a, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    for (size_t i = 0; i < ARRAYLEN(builtin_configs); i++) {
        const char *name = builtin_configs[i].name;
        if (str_has_strn_prefix(name, prefix, prefix_len)) {
            ptr_array_append(a, xstrdup(name));
        }
    }
}

void collect_builtin_includes(PointerArray *a, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    for (size_t i = 0; i < ARRAYLEN(builtin_configs); i++) {
        const char *name = builtin_configs[i].name;
        if (
            str_has_strn_prefix(name, prefix, prefix_len)
            && !str_has_prefix(name, "syntax/")
        ) {
            ptr_array_append(a, xstrdup(name));
        }
    }
}

UNITTEST {
    // NOLINTBEGIN(bugprone-assert-side-effect)
    BUG_ON(!get_builtin_config("rc"));
    BUG_ON(!get_builtin_config("color/reset"));
    // NOLINTEND(bugprone-assert-side-effect)
}
