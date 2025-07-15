#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "file-option.h"
#include "command/serialize.h"
#include "editor.h"
#include "editorconfig/editorconfig.h"
#include "options.h"
#include "regexp.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/str-array.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

typedef struct {
    FileOptionType type;
    char **strs;
    FileTypeOrFileName u;
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

    EditorConfigIndentStyle style = opts.indent_style;
    if (style != INDENT_STYLE_UNSPECIFIED) {
        options->expand_tab = (style == INDENT_STYLE_SPACE);
        options->emulate_tab = (style == INDENT_STYLE_SPACE);
        options->detect_indent = 0;
    }

    unsigned int iw = opts.indent_size;
    if (iw && iw <= INDENT_WIDTH_MAX) {
        options->indent_width = iw;
        options->detect_indent = 0;
    }

    unsigned int tw = opts.tab_width;
    if (tw && tw <= TAB_WIDTH_MAX) {
        options->tab_width = tw;
    }

    unsigned int maxlen = opts.max_line_length;
    if (maxlen && maxlen <= TEXT_WIDTH_MAX) {
        options->text_width = maxlen;
    }
}

void set_file_options(EditorState *e, Buffer *buffer)
{
    const PointerArray *fileopts = &e->file_options;
    const char *buffer_filetype = buffer->options.filetype;

    for (size_t i = 0, n = fileopts->count; i < n; i++) {
        const FileOption *opt = fileopts->ptrs[i];
        if (opt->type == FOPTS_FILETYPE) {
            if (interned_strings_equal(opt->u.filetype, buffer_filetype)) {
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
        if (regexp_exec(re, filename, strlen(filename), 0, NULL, 0)) {
            set_options(e, opt->strs);
        }
    }
}

void add_file_options (
    PointerArray *file_options,
    FileOptionType type,
    FileTypeOrFileName u,
    char **strs,
    size_t nstrs
) {
    BUG_ON(nstrs < 2);
    if (type == FOPTS_FILENAME) {
        BUG_ON(!u.filename);
        BUG_ON(!u.filename->str);
    } else {
        BUG_ON(type != FOPTS_FILETYPE);
        BUG_ON(!u.filetype);
    }

    FileOption *opt = xmalloc(sizeof(*opt));
    opt->u = u;
    opt->type = type;
    opt->strs = copy_string_array(strs, nstrs);
    ptr_array_append(file_options, opt);
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
    free_string_array(opt->strs);
    free(opt);
}

void free_file_options(PointerArray *file_options)
{
    ptr_array_free_cb(file_options, FREE_FUNC(free_file_option));
}
