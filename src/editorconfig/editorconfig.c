#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include "editorconfig.h"
#include "ini.h"
#include "match.h"
#include "../common.h"
#include "../debug.h"
#include "../util/ascii.h"
#include "../util/path.h"
#include "../util/string-view.h"
#include "../util/strtonum.h"
#include "../util/xmalloc.h"

typedef struct {
    const char *full_filename;
    StringView config_file_dir;
    EditorConfigOptions options;
} CallbackData;

static inline bool streq_icase(const char *a, const char *b)
{
    return strcasecmp(a, b) == 0;
}

static void editorconfig_option_set (
    EditorConfigOptions *options,
    const char *name,
    const char *val
) {
    if (streq_icase(name, "indent_style")) {
        if (streq_icase(val, "space")) {
            options->indent_style = INDENT_STYLE_SPACE;
        } else if (streq_icase(val, "tab")) {
            options->indent_style = INDENT_STYLE_TAB;
        }
    } else if (streq_icase(name, "indent_size")) {
        unsigned int n;
        if (streq_icase(val, "tab")) {
            options->indent_size_is_tab = true;
            options->indent_size = 0;
        } else if (str_to_uint(val, &n)) {
            options->indent_size_is_tab = false;
            options->indent_size = n;
        }
    } else if (streq_icase(name, "tab_width")) {
        unsigned int n;
        if (str_to_uint(val, &n)) {
            options->tab_width = n;
        }
    } else if (streq_icase(name, "max_line_length")) {
        unsigned int n;
        if (str_to_uint(val, &n)) {
            options->max_line_length = n;
        }
    }
}

static int ini_handler (
    void *ud,
    const char *section,
    const char *name,
    const char *value
) {
    CallbackData *data = ud;

    if (
        section[0] == '\0'
        && streq_icase(name, "root")
        && streq_icase(value, "true")
    ) {
        // root=true, clear all previous values
        memzero(&data->options);
        return 1;
    }

    const size_t section_len = strlen(section);
    char *pattern = xmalloc (
        2 * data->config_file_dir.length
        + sizeof("**/")
        + section_len
    );

    // Escape editorconfig special chars in path
    const StringView ecfile_dir = data->config_file_dir;
    size_t j = 0;
    for (size_t i = 0, n = ecfile_dir.length; i < n; i++) {
        const char ch = ecfile_dir.data[i];
        switch (ch) {
        case '*': case ',': case '-':
        case '?': case '[': case '\\':
        case ']': case '{': case '}':
            pattern[j++] = '\\';
            // Fallthrough
        default:
            pattern[j++] = ch;
        }
    }

    if (strchr(section, '/') == NULL) {
        // No slash in pattern, append "**/"
        pattern[j++] = '*';
        pattern[j++] = '*';
        pattern[j++] = '/';
    } else if (section[0] != '/') {
        // Pattern contains at least one slash but not at the start, add one
        pattern[j++] = '/';
    }

    memcpy(pattern + j, section, section_len + 1); // +1 for NUL

    if (ec_pattern_match(pattern, data->full_filename)) {
        editorconfig_option_set(&data->options, name, value);
    }

    free(pattern);
    return 1;
}

int editorconfig_parse(const char *full_filename, EditorConfigOptions *opts)
{
    BUG_ON(full_filename[0] != '/');
    CallbackData data = {
        .full_filename = full_filename
    };

    char buf[8192];
    size_t dir_len;
    const char *slash = strrchr(full_filename, '/');
    BUG_ON(slash == NULL);
    if (slash == full_filename) {
        dir_len = 0;
    } else {
        dir_len = slash - full_filename;
        memcpy(buf, full_filename, dir_len);
    }
    memcpy(buf + dir_len, "/.editorconfig", 15);

    // Iterate up directory tree, looking for ".editorconfig" at each level
    while (1) {
        data.config_file_dir = string_view(buf, dir_len);
        int err_num = ini_parse(buf, ini_handler, &data);
        if (err_num > 0) {
            return err_num;
        }

        buf[dir_len] = '\0';
        slash = memrchr_(buf, '/', dir_len);
        if (slash == NULL) {
            break;
        }
        dir_len = slash - buf;
        memcpy(buf + dir_len, "/.editorconfig", 15);
    }

    // Set indent_size to "tab" if indent_size is not specified and
    // indent_style is set to "tab".
    if (
        data.options.indent_size == 0
        && data.options.indent_style == INDENT_STYLE_TAB
    ) {
        data.options.indent_size_is_tab = true;
    }

    // Set indent_size to tab_width if indent_size is "tab" and
    // tab_width is specified.
    if (data.options.indent_size_is_tab && data.options.tab_width > 0) {
        data.options.indent_size = data.options.tab_width;
    }

    // Set tab_width to indent_size if indent_size is specified as
    // something other than "tab" and tab_width is unspecified
    if (
        data.options.indent_size != 0
        && data.options.tab_width == 0
        && !data.options.indent_size_is_tab
    ) {
        data.options.tab_width = data.options.indent_size;
    }

    *opts = data.options;
    return 0;
}
