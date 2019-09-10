#include <unistd.h>
#include "file-option.h"
#include "editorconfig/editorconfig.h"
#include "options.h"
#include "regexp.h"
#include "spawn.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"

typedef struct {
    FileOptionType type;
    char *type_or_pattern;
    char **strs;
} FileOption;

static PointerArray file_options = PTR_ARRAY_INIT;

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
    if (path == NULL) {
        // For buffers with no associated filename, use a dummy path of
        // "$PWD/_", to obtain generic settings for the working directory
        // or the user's default settings.
        if (getcwd(cwd, sizeof(cwd) - 2) == NULL) {
            return;
        }
        size_t n = strlen(cwd);
        cwd[n++] = '/';
        cwd[n++] = '_';
        cwd[n] = '\0';
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
    if (indent_size > 0 && indent_size <= 8) {
        b->options.indent_width = indent_size;
        b->options.detect_indent = 0;
    }

    const unsigned int tab_width = opts.tab_width;
    if (tab_width > 0 && tab_width <= 8) {
        b->options.tab_width = tab_width;
    }

    const unsigned int max_line_length = opts.max_line_length;
    if (max_line_length > 0 && max_line_length <= 1000) {
        b->options.text_width = max_line_length;
    }
}

void set_file_options(Buffer *b)
{
    for (size_t i = 0; i < file_options.count; i++) {
        const FileOption *opt = file_options.ptrs[i];
        if (opt->type == FILE_OPTIONS_FILETYPE) {
            if (streq(opt->type_or_pattern, b->options.filetype)) {
                set_options(opt->strs);
            }
            continue;
        }
        const char *f = b->abs_filename;
        if (f && regexp_match_nosub(opt->type_or_pattern, f, strlen(f))) {
            set_options(opt->strs);
        }
    }
}

void add_file_options(FileOptionType type, char *to, char **strs)
{
    if (type == FILE_OPTIONS_FILENAME && !regexp_is_valid(to, REG_NEWLINE)) {
        free(to);
        free_string_array(strs);
        return;
    }

    FileOption *opt = xnew(FileOption, 1);
    opt->type = type;
    opt->type_or_pattern = to;
    opt->strs = strs;
    ptr_array_add(&file_options, opt);
}
