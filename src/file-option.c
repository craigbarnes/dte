#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "file-option.h"
#include "command/serialize.h"
#include "editor.h"
#include "editorconfig/editorconfig.h"
#include "error.h"
#include "options.h"
#include "regexp.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

typedef struct {
    FileOptionType type;
    char **strs;
    union {
        char *filetype;
        CachedRegexp *filename;
    } u;
} FileOption;

static void set_options(EditorState *e, char **args)
{
    for (size_t i = 0; args[i]; i += 2) {
        set_option(e, args[i], args[i + 1], true, false);
    }
}

void set_editorconfig_options(Buffer *buffer)
{
    LocalOptions *options = &buffer->options;
    if (!options->editorconfig) {
        return;
    }

    const char *path = buffer->abs_filename;
    char cwd[8192];
    if (!path) {
        // For buffers with no associated filename, use a dummy path of
        // "$PWD/__", to obtain generic settings for the working directory
        // or the user's default settings
        static const char suffix[] = "/__";
        if (unlikely(!getcwd(cwd, sizeof(cwd) - sizeof(suffix)))) {
            return;
        }
        memcpy(cwd + strlen(cwd), suffix, sizeof(suffix));
        path = cwd;
    }

    EditorConfigOptions opts;
    if (get_editorconfig_options(path, &opts) != 0) {
        return;
    }

    switch (opts.indent_style) {
    case INDENT_STYLE_SPACE:
        options->expand_tab = true;
        options->emulate_tab = true;
        options->detect_indent = 0;
        break;
    case INDENT_STYLE_TAB:
        options->expand_tab = false;
        options->emulate_tab = false;
        options->detect_indent = 0;
        break;
    case INDENT_STYLE_UNSPECIFIED:
        break;
    }

    const unsigned int indent_size = opts.indent_size;
    if (indent_size > 0 && indent_size <= INDENT_WIDTH_MAX) {
        options->indent_width = indent_size;
        options->detect_indent = 0;
    }

    const unsigned int tab_width = opts.tab_width;
    if (tab_width > 0 && tab_width <= TAB_WIDTH_MAX) {
        options->tab_width = tab_width;
    }

    const unsigned int max_line_length = opts.max_line_length;
    if (max_line_length > 0 && max_line_length <= TEXT_WIDTH_MAX) {
        options->text_width = max_line_length;
    }
}

void set_file_options(EditorState *e, Buffer *buffer)
{
    for (size_t i = 0, n = e->file_options.count; i < n; i++) {
        const FileOption *opt = e->file_options.ptrs[i];
        if (opt->type == FOPTS_FILETYPE) {
            if (streq(opt->u.filetype, buffer->options.filetype)) {
                set_options(e, opt->strs);
            }
            continue;
        }

        BUG_ON(opt->type != FOPTS_FILENAME);
        const char *filename = buffer->abs_filename;
        if (!filename) {
            continue;
        }

        const regex_t *re = &opt->u.filename->re;
        regmatch_t m;
        if (regexp_exec(re, filename, strlen(filename), 0, &m, 0)) {
            set_options(e, opt->strs);
        }
    }
}

bool add_file_options(PointerArray *file_options, FileOptionType type, StringView str, char **strs, size_t nstrs)
{
    size_t len = str.length;
    if (unlikely(len == 0)) {
        const char *desc = (type == FOPTS_FILETYPE) ? "filetype" : "pattern";
        return error_msg("can't add option with empty %s", desc);
    }

    FileOption *opt = xnew(FileOption, 1);
    if (type == FOPTS_FILETYPE) {
        opt->u.filetype = xstrcut(str.data, len);
        goto append;
    }

    BUG_ON(type != FOPTS_FILENAME);
    CachedRegexp *r = xmalloc(sizeof(*r) + len + 1);
    memcpy(r->str, str.data, len);
    r->str[len] = '\0';
    opt->u.filename = r;

    int err = regcomp(&r->re, r->str, DEFAULT_REGEX_FLAGS | REG_NEWLINE | REG_NOSUB);
    if (unlikely(err)) {
        regexp_error_msg(&r->re, r->str, err);
        free(r);
        free(opt);
        return false;
    }

append:
    opt->type = type;
    opt->strs = copy_string_array(strs, nstrs);
    ptr_array_append(file_options, opt);
    return true;
}

void dump_file_options(const PointerArray *file_options, String *buf)
{
    for (size_t i = 0, n = file_options->count; i < n; i++) {
        const FileOption *opt = file_options->ptrs[i];
        const char *tp;
        if (opt->type == FOPTS_FILENAME) {
            tp = opt->u.filename->str;
        } else {
            tp = opt->u.filetype;
        }
        char **strs = opt->strs;
        string_append_literal(buf, "option ");
        if (opt->type == FOPTS_FILENAME) {
            string_append_literal(buf, "-r ");
        }
        if (str_has_prefix(tp, "-") || string_array_contains_prefix(strs, "-")) {
            string_append_literal(buf, "-- ");
        }
        string_append_escaped_arg(buf, tp, true);
        for (size_t j = 0; strs[j]; j += 2) {
            string_append_byte(buf, ' ');
            string_append_cstring(buf, strs[j]);
            string_append_byte(buf, ' ');
            string_append_escaped_arg(buf, strs[j + 1], true);
        }
        string_append_byte(buf, '\n');
    }
}

static void free_file_option(FileOption *opt)
{
    if (opt->type == FOPTS_FILENAME) {
        free_cached_regexp(opt->u.filename);
    } else {
        BUG_ON(opt->type != FOPTS_FILETYPE);
        free(opt->u.filetype);
    }
    free_string_array(opt->strs);
    free(opt);
}

void free_file_options(PointerArray *file_options)
{
    ptr_array_free_cb(file_options, FREE_FUNC(free_file_option));
}
