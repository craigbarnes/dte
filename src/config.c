#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include "config.h"
#include "commands.h"
#include "completion.h"
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

static bool is_command(const char *str, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '#') {
            return false;
        }
        if (!ascii_isspace(str[i])) {
            return true;
        }
    }
    return false;
}

// Odd number of backslashes at end of line?
static bool has_line_continuation(const char *str, size_t len)
{
    ssize_t pos = len - 1;
    while (pos >= 0 && str[pos] == '\\') {
        pos--;
    }
    return (len - 1 - pos) % 2;
}

void exec_config(const CommandSet *cmds, const char *buf, size_t size)
{
    const char *ptr = buf;
    String line = string_new(1024);

    while (ptr < buf + size) {
        size_t n = buf + size - ptr;
        char *end = memchr(ptr, '\n', n);
        if (end) {
            n = end - ptr;
        }

        if (line.len || is_command(ptr, n)) {
            if (has_line_continuation(ptr, n)) {
                string_append_buf(&line, ptr, n - 1);
            } else {
                string_append_buf(&line, ptr, n);
                handle_command(cmds, string_borrow_cstring(&line), false);
                string_clear(&line);
            }
        }

        current_config.line++;
        ptr += n + 1;
    }

    if (line.len) {
        handle_command(cmds, string_borrow_cstring(&line), false);
    }

    string_free(&line);
}

String dump_builtin_configs(void)
{
    String str = string_new(1024);
    for (size_t i = 0; i < ARRAY_COUNT(builtin_configs); i++) {
        string_append_cstring(&str, builtin_configs[i].name);
        string_append_byte(&str, '\n');
    }
    return str;
}

void collect_builtin_configs(const char *prefix)
{
    for (size_t i = 0; i < ARRAY_COUNT(builtin_configs); i++) {
        const BuiltinConfig *cfg = &builtin_configs[i];
        if (str_has_prefix(cfg->name, "syntax/")) {
            return;
        } else if (str_has_prefix(cfg->name, prefix)) {
            add_completion(xstrdup(cfg->name));
        }
    }
}

const BuiltinConfig *get_builtin_config(const char *name)
{
    for (size_t i = 0; i < ARRAY_COUNT(builtin_configs); i++) {
        if (streq(name, builtin_configs[i].name)) {
            return &builtin_configs[i];
        }
    }
    return NULL;
}

const BuiltinConfig *get_builtin_configs_array(size_t *nconfigs)
{
    *nconfigs = ARRAY_COUNT(builtin_configs);
    return &builtin_configs[0];
}

int do_read_config(const CommandSet *cmds, const char *filename, ConfigFlags flags)
{
    const bool must_exist = flags & CFG_MUST_EXIST;
    const bool builtin = flags & CFG_BUILTIN;

    if (builtin) {
        const BuiltinConfig *cfg = get_builtin_config(filename);
        if (cfg) {
            current_config.file = filename;
            current_config.line = 1;
            exec_config(cmds, cfg->text.data, cfg->text.length);
            return 0;
        } else if (must_exist) {
            error_msg (
                "Error reading '%s': no built-in config exists for that path",
                filename
            );
            return 1;
        } else {
            return 0;
        }
    }

    char *buf;
    ssize_t size = read_file(filename, &buf);
    int err = errno;

    if (size < 0) {
        if (err != ENOENT || must_exist) {
            error_msg("Error reading %s: %s", filename, strerror(err));
        }
        return err;
    }

    current_config.file = filename;
    current_config.line = 1;

    exec_config(cmds, buf, size);
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
    clear_hl_colors();
    bool colors = terminal.color_type >= TERM_8_COLOR;
    const char *cfg = colors ? "color/reset" : "color/reset-basic";
    read_config(&commands, cfg, CFG_MUST_EXIST | CFG_BUILTIN);
}

void exec_builtin_rc(void)
{
    exec_builtin_color_reset();
    read_config(&commands, "rc", CFG_MUST_EXIST | CFG_BUILTIN);
}

UNITTEST {
    // Built-in configs can be customized, but these 3 are required:
    BUG_ON(!get_builtin_config("rc"));
    BUG_ON(!get_builtin_config("color/reset"));
    BUG_ON(!get_builtin_config("color/reset-basic"));
}
