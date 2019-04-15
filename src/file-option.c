#include "file-option.h"
#include "options.h"
#include "util/ptr-array.h"
#include "util/regexp.h"
#include "util/string-view.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "spawn.h"

typedef struct {
    enum file_options_type type;
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

    const char *cmd[] = {"editorconfig", path, NULL};
    FilterData data = FILTER_DATA_INIT;
    if (spawn_filter((char**)cmd, &data) != 0 || data.out_len == 0) {
        return;
    }

    ssize_t pos = 0, size = data.out_len;
    while (pos < size) {
        const char *const line = buf_next_line(data.out, &pos, size);
        const char *const delim = strchr(line, '=');
        if (delim == NULL || delim == line || delim[1] == '\0') {
            continue;
        }

        const StringView key = string_view(line, (size_t)(delim - line));
        const char *const val = delim + 1;

        if (string_view_equal_literal(&key, "indent_style")) {
            if (streq(val, "spaces")) {
                b->options.expand_tab = true;
                b->options.emulate_tab = true;
                b->options.detect_indent = 0;
            } else if (streq(val, "tabs")) {
                b->options.expand_tab = false;
                b->options.emulate_tab = false;
                b->options.detect_indent = 0;
            }
        } else if (string_view_equal_literal(&key, "indent_size")) {
            unsigned int n;
            if (str_to_uint(val, &n) && n > 0 && n <= 8) {
                b->options.indent_width = n;
                b->options.detect_indent = 0;
            }
        } else if (string_view_equal_literal(&key, "tab_width")) {
            unsigned int n;
            if (str_to_uint(val, &n) && n > 0 && n <= 8) {
                b->options.tab_width = n;
            }
        }
    }

    free(data.out);
}

void set_file_options(Buffer *b)
{
    for (size_t i = 0; i < file_options.count; i++) {
        const FileOption *opt = file_options.ptrs[i];

        if (opt->type == FILE_OPTIONS_FILETYPE) {
            if (streq(opt->type_or_pattern, b->options.filetype)) {
                set_options(opt->strs);
            }
        } else if (
            b->abs_filename
            && regexp_match_nosub (
                opt->type_or_pattern,
                b->abs_filename,
                strlen(b->abs_filename)
            )
        ) {
            set_options(opt->strs);
        }
    }
}

void add_file_options(enum file_options_type type, char *to, char **strs)
{
    FileOption *opt;
    regex_t re;

    if (type == FILE_OPTIONS_FILENAME) {
        if (!regexp_compile(&re, to, REG_NEWLINE | REG_NOSUB)) {
            free(to);
            free_strings(strs);
            return;
        }
        regfree(&re);
    }

    opt = xnew(FileOption, 1);
    opt->type = type;
    opt->type_or_pattern = to;
    opt->strs = strs;
    ptr_array_add(&file_options, opt);
}
