#include "feature.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "config.h"
#include "command/cache.h"
#include "command/error.h"
#include "commands.h"
#include "compiler.h"
#include "editor.h"
#include "syntax/color.h"
#include "util/debug.h"
#include "util/hashmap.h"
#include "util/intmap.h"
#include "util/log.h"
#include "util/readfile.h"
#include "util/str-util.h"
#include "util/xsnprintf.h"

#if HAVE_EMBED
    #include "builtin-config-embed.h"
#else
    #include "builtin-config.h"
#endif

UNITTEST {
    // NOLINTBEGIN(bugprone-assert-side-effect)
    BUG_ON(!get_builtin_config("rc"));
    BUG_ON(!get_builtin_config("color/reset"));
    // NOLINTEND(bugprone-assert-side-effect)
}

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

bool exec_config(CommandRunner *runner, StringView config)
{
    EditorState *e = runner->e;
    ErrorBuffer *ebuf = runner->ebuf;
    if (unlikely(e->include_recursion_count > 64)) {
        return error_msg_for_cmd(ebuf, NULL, "config recursion limit reached");
    }

    bool stop_at_first_err = (runner->flags & CMDRUNNER_STOP_AT_FIRST_ERROR);
    size_t nfailed = 0;
    String buf = string_new(1024);
    e->include_recursion_count++;

    for (size_t i = 0, n = config.length; i < n; ebuf->config_line++) {
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
            bool r = handle_command(runner, string_borrow_cstring(&buf));
            string_clear(&buf);
            nfailed += !r;
            if (unlikely(!r && stop_at_first_err)) {
                goto out;
            }
        }
    }

    if (unlikely(buf.len)) {
        // This can only happen if the last line had a line continuation
        bool r = handle_command(runner, string_borrow_cstring(&buf));
        nfailed += !r;
    }

out:
    e->include_recursion_count--;
    string_free(&buf);
    return (nfailed == 0);
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

static int do_read_config(CommandRunner *runner, const char *filename, ConfigFlags flags)
{
    ErrorBuffer *ebuf = runner->ebuf;
    bool must_exist = flags & CFG_MUST_EXIST;
    bool stop_at_first_err = runner->flags & CMDRUNNER_STOP_AT_FIRST_ERROR;

    if (flags & CFG_BUILTIN) {
        const BuiltinConfig *cfg = get_builtin_config(filename);
        if (cfg) {
            ebuf->config_filename = filename;
            ebuf->config_line = 1;
            bool r = exec_config(runner, cfg->text);
            return (r || !stop_at_first_err) ? 0 : EINVAL;
        }
        if (must_exist) {
            error_msg(ebuf, "no built-in config with name '%s'", filename);
        }
        return ENOENT;
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
    bool r = exec_config(runner, string_view(buf, size));
    free(buf);
    return (r || !stop_at_first_err) ? 0 : EINVAL;
}

int read_config(CommandRunner *runner, const char *filename, ConfigFlags flags)
{
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

static void log_config_counts(const EditorState *e)
{
    if (!log_level_enabled(LOG_LEVEL_INFO)) {
        return;
    }

    size_t nbinds = 0;
    size_t nbinds_cached = 0;
    for (HashMapIter modeiter = hashmap_iter(&e->modes); hashmap_next(&modeiter); ) {
        const ModeHandler *mode = modeiter.entry->value;
        const IntMap *binds = &mode->key_bindings;
        nbinds += binds->count;
        for (IntMapIter binditer = intmap_iter(binds); intmap_next(&binditer); ) {
            const CachedCommand *cc = binditer.entry->value;
            nbinds_cached += !!cc->cmd;
        }
    }

    size_t nerrorfmts = 0;
    for (HashMapIter it = hashmap_iter(&e->compilers); hashmap_next(&it); ) {
        const Compiler *compiler = it.entry->value;
        nerrorfmts += compiler->error_formats.count;
    }

    LOG_INFO (
        "bind=%zu(%zu) alias=%zu hi=%zu ft=%zu option=%zu errorfmt=%zu(%zu)",
        nbinds,
        nbinds_cached,
        e->aliases.count,
        e->styles.other.count + NR_BSE,
        e->filetypes.count,
        e->file_options.count,
        e->compilers.count,
        nerrorfmts
    );
}

void exec_rc_files(EditorState *e, const char *filename, bool read_user_rc)
{
    StringView rc = string_view(builtin_rc, sizeof(builtin_rc) - 1);
    exec_builtin_color_reset(e);
    exec_builtin_config(e, rc, "rc");

    if (read_user_rc) {
        ConfigFlags flags = CFG_NOFLAGS;
        char buf[8192];
        if (filename) {
            flags |= CFG_MUST_EXIST;
        } else {
            xsnprintf(buf, sizeof buf, "%s/%s", e->user_config_dir, "rc");
            filename = buf;
        }
        LOG_INFO("loading configuration from %s", filename);
        read_normal_config(e, filename, flags);
    }

    log_config_counts(e);
    update_all_syntax_styles(&e->syntaxes, &e->styles);
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
            && !str_has_prefix(name, "script/")
        ) {
            ptr_array_append(a, xstrdup(name));
        }
    }
}
