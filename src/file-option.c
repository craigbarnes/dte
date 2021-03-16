#include <unistd.h>
#include "file-option.h"
#include "command/serialize.h"
#include "editorconfig/editorconfig.h"
#include "options.h"
#include "regexp.h"
#include "util/debug.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
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
    if (!path) {
        // For buffers with no associated filename, use a dummy path of
        // "$PWD/__", to obtain generic settings for the working directory
        // or the user's default settings.
        static const char suffix[] = "/__";
        if (!getcwd(cwd, sizeof(cwd) - sizeof(suffix))) {
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
    for (size_t i = 0, n = file_options.count; i < n; i++) {
        const FileOption *opt = file_options.ptrs[i];
        switch (opt->type) {
        case FILE_OPTIONS_FILETYPE:
            if (streq(opt->type_or_pattern, b->options.filetype)) {
                set_options(opt->strs);
            }
            break;
        case FILE_OPTIONS_FILENAME:
            if (b->abs_filename) {
                const StringView f = strview_from_cstring(b->abs_filename);
                if (regexp_match_nosub(opt->type_or_pattern, &f)) {
                    set_options(opt->strs);
                }
            }
            break;
        default:
            BUG("unhandled file option type");
        }
    }
}

void add_file_options(FileOptionType type, char *to, char **strs)
{
    if (
        (type == FILE_OPTIONS_FILENAME && !regexp_is_valid(to, REG_NEWLINE))
        || (type == FILE_OPTIONS_FILETYPE && to[0] == '\0')
    ) {
        free(to);
        free_string_array(strs);
        return;
    }

    FileOption *opt = xnew(FileOption, 1);
    opt->type = type;
    opt->type_or_pattern = to;
    opt->strs = strs;
    ptr_array_append(&file_options, opt);
}

void dump_file_options(String *buf)
{
    for (size_t i = 0, n = file_options.count; i < n; i++) {
        const FileOption *opt = file_options.ptrs[i];
        const char *tp = opt->type_or_pattern;
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
