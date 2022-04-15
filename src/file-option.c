#include <unistd.h>
#include "file-option.h"
#include "command/serialize.h"
#include "editorconfig/editorconfig.h"
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

static void set_options(char **args)
{
    for (size_t i = 0; args[i]; i += 2) {
        set_option(args[i], args[i + 1], true, false);
    }
}

void set_editorconfig_options(Buffer *b)
{
    if (!b->options.editorconfig) {
        return;
    }

    const char *path = b->abs_filename;
    char cwd[8192];
    if (!path) {
        // For buffers with no associated filename, use a dummy path of
        // "$PWD/__", to obtain generic settings for the working directory
        // or the user's default settings.
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
        b->options.expand_tab = true;
        b->options.emulate_tab = true;
        b->options.detect_indent = 0;
        break;
    case INDENT_STYLE_TAB:
        b->options.expand_tab = false;
        b->options.emulate_tab = false;
        b->options.detect_indent = 0;
        break;
    case INDENT_STYLE_UNSPECIFIED:
        break;
    }

    const unsigned int indent_size = opts.indent_size;
    if (indent_size > 0 && indent_size <= INDENT_WIDTH_MAX) {
        b->options.indent_width = indent_size;
        b->options.detect_indent = 0;
    }

    const unsigned int tab_width = opts.tab_width;
    if (tab_width > 0 && tab_width <= TAB_WIDTH_MAX) {
        b->options.tab_width = tab_width;
    }

    const unsigned int max_line_length = opts.max_line_length;
    if (max_line_length > 0 && max_line_length <= TEXT_WIDTH_MAX) {
        b->options.text_width = max_line_length;
    }
}

void set_file_options(const PointerArray *file_options, Buffer *b)
{
    for (size_t i = 0, n = file_options->count; i < n; i++) {
        const FileOption *opt = file_options->ptrs[i];
        if (opt->type == FILE_OPTIONS_FILETYPE) {
            if (streq(opt->u.filetype, b->options.filetype)) {
                set_options(opt->strs);
            }
            continue;
        }

        BUG_ON(opt->type != FILE_OPTIONS_FILENAME);
        const char *filename = b->abs_filename;
        if (!filename) {
            continue;
        }

        const regex_t *re = &opt->u.filename->re;
        regmatch_t m;
        if (regexp_exec(re, filename, strlen(filename), 0, &m, 0)) {
            set_options(opt->strs);
        }
    }
}

void add_file_options(PointerArray *file_options, FileOptionType type, StringView str, char **strs, size_t nstrs)
{
    FileOption *opt = xnew(FileOption, 1);
    size_t len = str.length;
    if (type == FILE_OPTIONS_FILETYPE) {
        if (unlikely(len == 0)) {
            goto error;
        }
        opt->u.filetype = xstrcut(str.data, len);
        goto append;
    }

    BUG_ON(type != FILE_OPTIONS_FILENAME);
    CachedRegexp *r = xmalloc(sizeof(*r) + len + 1);
    memcpy(r->str, str.data, len);
    r->str[len] = '\0';
    opt->u.filename = r;

    int err = regcomp(&r->re, r->str, REG_EXTENDED | REG_NEWLINE | REG_NOSUB);
    if (unlikely(err)) {
        regexp_error_msg(&r->re, r->str, err);
        free(r);
        goto error;
    }

append:
    opt->type = type;
    opt->strs = copy_string_array(strs, nstrs);
    ptr_array_append(file_options, opt);
    return;

error:
    free(opt);
}

void dump_file_options(const PointerArray *file_options, String *buf)
{
    for (size_t i = 0, n = file_options->count; i < n; i++) {
        const FileOption *opt = file_options->ptrs[i];
        const char *tp;
        if (opt->type == FILE_OPTIONS_FILENAME) {
            tp = opt->u.filename->str;
        } else {
            tp = opt->u.filetype;
        }
        char **strs = opt->strs;
        string_append_literal(buf, "option ");
        if (opt->type == FILE_OPTIONS_FILENAME) {
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
    if (opt->type == FILE_OPTIONS_FILENAME) {
        free_cached_regexp(opt->u.filename);
    } else {
        BUG_ON(opt->type != FILE_OPTIONS_FILETYPE);
        free(opt->u.filetype);
    }
    free_string_array(opt->strs);
    free(opt);
}

void free_file_options(PointerArray *file_options)
{
    ptr_array_free_cb(file_options, (FreeFunction)free_file_option);
}
