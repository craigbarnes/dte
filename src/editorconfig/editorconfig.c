#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "editorconfig.h"
#include "ini.h"
#include "match.h"
#include "options.h"
#include "util/debug.h"
#include "util/path.h"
#include "util/readfile.h"
#include "util/string.h"
#include "util/string-view.h"
#include "util/strtonum.h"
#include "util/xstring.h"

enum {
    MAX_FILESIZE = 32u << 20, // 32 MiB
};

typedef struct {
    const char *const pathname;
    StringView config_file_dir;
    EditorConfigOptions options;
    bool match;
} UserData;

typedef enum {
    ECONF_CHARSET,
    ECONF_END_OF_LINE,
    ECONF_INDENT_SIZE,
    ECONF_INDENT_STYLE,
    ECONF_INSERT_FINAL_NL,
    ECONF_MAX_LINE_LENGTH,
    ECONF_TAB_WIDTH,
    ECONF_TRIM_TRAILING_WS,
    ECONF_UNKNOWN_PROPERTY,
} PropertyType;

#define CMP(s, val) if (mem_equal(name->data, s, STRLEN(s))) return val;

static PropertyType lookup_property(const StringView *name)
{
    switch (name->length) {
    case  7: CMP("charset", ECONF_CHARSET); break;
    case  9: CMP("tab_width", ECONF_TAB_WIDTH); break;
    case 11:
        CMP("indent_size", ECONF_INDENT_SIZE);
        CMP("end_of_line", ECONF_END_OF_LINE);
        break;
    case 12: CMP("indent_style", ECONF_INDENT_STYLE); break;
    case 15: CMP("max_line_length", ECONF_MAX_LINE_LENGTH); break;
    case 20: CMP("insert_final_newline", ECONF_INSERT_FINAL_NL); break;
    case 24: CMP("trim_trailing_whitespace", ECONF_TRIM_TRAILING_WS); break;
    }
    return ECONF_UNKNOWN_PROPERTY;
}

static EditorConfigIndentStyle lookup_indent_style(const StringView *val)
{
    if (strview_equal_cstring_icase(val, "space")) {
        return INDENT_STYLE_SPACE;
    } else if (strview_equal_cstring_icase(val, "tab")) {
        return INDENT_STYLE_TAB;
    }
    return INDENT_STYLE_UNSPECIFIED;
}

static unsigned int parse_indent_digit(const StringView *val)
{
    static_assert(INDENT_WIDTH_MAX == TAB_WIDTH_MAX);
    const char *data = val->data;
    unsigned int indent = (val->length == 1) ? data[0] - '0' : 0;
    return (indent <= INDENT_WIDTH_MAX) ? indent : 0;
}

static void parse_indent_size(EditorConfigOptions *options, const StringView *val)
{
    bool tab = strview_equal_cstring_icase(val, "tab");
    options->indent_size_is_tab = tab;
    options->indent_size = tab ? 0 : parse_indent_digit(val);
}

static unsigned int parse_max_line_length(const StringView *val)
{
    unsigned int maxlen = 0;
    size_t ndigits = buf_parse_uint(val->data, val->length, &maxlen);
    return (ndigits == val->length) ? MIN(maxlen, TEXT_WIDTH_MAX) : 0;
}

static void editorconfig_option_set (
    EditorConfigOptions *options,
    const StringView *name,
    const StringView *val
) {
    switch (lookup_property(name)) {
    case ECONF_INDENT_STYLE:
        options->indent_style = lookup_indent_style(val);
        break;
    case ECONF_INDENT_SIZE:
        parse_indent_size(options, val);
        break;
    case ECONF_TAB_WIDTH:
        options->tab_width = parse_indent_digit(val);
        break;
    case ECONF_MAX_LINE_LENGTH:
        options->max_line_length = parse_max_line_length(val);
        break;
    case ECONF_CHARSET:
    case ECONF_END_OF_LINE:
    case ECONF_INSERT_FINAL_NL:
    case ECONF_TRIM_TRAILING_WS:
    case ECONF_UNKNOWN_PROPERTY:
        break;
    default:
        BUG("unhandled property type");
    }
}

// See: https://editorconfig.org/#wildcards and ec_pattern_match()
static bool is_ec_special_char(char c)
{
    return c == '*' || c == ',' || c == '-' || c == '?' || c == '['
        || c == ']' || c == '{' || c == '}' || c == '\\';
}

static bool section_matches_path(StringView section, StringView dir, const char *path)
{
    BUG_ON(section.length == 0);
    String pattern = string_new(dir.length + section.length + 16);

    // Add `dir` prefix to `pattern`, escaping any special characters
    for (size_t i = 0, n = dir.length; i < n; i++) {
        const char c = dir.data[i];
        if (is_ec_special_char(c)) {
            string_append_byte(&pattern, '\\');
        }
        string_append_byte(&pattern, c);
    }

    if (!strview_memchr(&section, '/')) {
        // Section contains no slashes; insert "**/" between `dir` and `section`
        string_append_literal(&pattern, "**/");
    } else if (section.data[0] != '/') {
        // Section contains slashes, but not at the start; insert '/' between
        // `dir` and `section`
        string_append_byte(&pattern, '/');
    }

    // Append the section heading (from the .editorconfig file) to the
    // prefix constructed above and test whether the resulting `pattern`
    // matches against `path`
    string_append_strview(&pattern, &section);
    bool r = ec_pattern_match(pattern.buffer, pattern.len, path);
    string_free(&pattern);
    return r;
}

static bool is_root_key(const IniParser *ini)
{
    return strview_equal_cstring_icase(&ini->name, "root")
        && strview_equal_cstring_icase(&ini->value, "true");
}

static EditorConfigOptions editorconfig_options_init(void)
{
    return (EditorConfigOptions){.indent_style = INDENT_STYLE_UNSPECIFIED};
}

static void editorconfig_parse(StringView input, UserData *data)
{
    static const StringView utf8_bom = STRING_VIEW("\xEF\xBB\xBF");
    bool has_utf8_bom = strview_has_sv_prefix(input, utf8_bom);

    IniParser ini = {
        .input = input,
        .pos = has_utf8_bom ? utf8_bom.length : 0,
    };

    while (ini_parse(&ini)) {
        if (ini.section.length == 0) {
            if (is_root_key(&ini)) {
                // root=true, clear all previous values
                data->options = editorconfig_options_init();
            }
            continue;
        }

        if (ini.name_count == 1) {
            // If name_count is 1, it indicates that the name/value pair is
            // the first in the section and therefore requires a new pattern
            // to be built and tested for a match
            StringView dir = data->config_file_dir;
            data->match = section_matches_path(ini.section, dir, data->pathname);
        } else {
            // Otherwise, the section is the same as was passed for the first
            // name/value pair in the section and the value of data->match
            // can just be reused
        }

        if (data->match) {
            editorconfig_option_set(&data->options, &ini.name, &ini.value);
        }
    }
}

EditorConfigOptions get_editorconfig_options(const char *pathname)
{
    BUG_ON(!path_is_absolute(pathname));
    UserData data = {
        .pathname = pathname,
        .config_file_dir = STRING_VIEW_INIT,
        .options = editorconfig_options_init(),
        .match = false,
    };

    static const char ecfilename[16] = "/.editorconfig";
    char buf[8192];
    memcpy(buf, ecfilename, sizeof ecfilename);

    const char *ptr = pathname + 1;
    size_t dir_len = 1;

    // Iterate up directory tree, looking for ".editorconfig" at each level
    while (1) {
        char *text;
        ssize_t len = read_file(buf, &text, MAX_FILESIZE);
        if (len >= 0) {
            data.config_file_dir = string_view(buf, dir_len);
            editorconfig_parse(string_view(text, len), &data);
            free(text);
        }

        const char *slash = strchr(ptr, '/');
        if (!slash) {
            break;
        }

        dir_len = slash - pathname;
        xmempcpy2(buf, pathname, dir_len, ecfilename, sizeof ecfilename);
        ptr = slash + 1;
    }

    // Set indent_size to "tab" if indent_size is not specified and
    // indent_style is set to "tab"
    EditorConfigOptions *o = &data.options;
    if (o->indent_size == 0 && o->indent_style == INDENT_STYLE_TAB) {
        o->indent_size_is_tab = true;
    }

    // Set indent_size to tab_width if indent_size is "tab" and
    // tab_width is specified
    if (o->indent_size_is_tab && o->tab_width > 0) {
        o->indent_size = o->tab_width;
    }

    // Set tab_width to indent_size if indent_size is specified as
    // something other than "tab" and tab_width is unspecified
    if (o->indent_size != 0 && o->tab_width == 0 && !o->indent_size_is_tab) {
        o->tab_width = o->indent_size;
    }

    return data.options;
}
