#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"
#include "buffer.h"
#include "command/serialize.h"
#include "completion.h"
#include "editor.h"
#include "error.h"
#include "file-option.h"
#include "filetype.h"
#include "regexp.h"
#include "screen.h"
#include "terminal/output.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/hashset.h"
#include "util/numtostr.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "view.h"
#include "window.h"

typedef enum {
    OPT_STR,
    OPT_UINT,
    OPT_ENUM,
    OPT_BOOL,
    OPT_FLAG,
} OptionType;

typedef struct {
    const char name[22];
    bool local;
    bool global;
    unsigned int offset;
    OptionType type;
    union {
        struct {
            // Optional
            bool (*validate)(const char *value);
        } str_opt;
        struct {
            unsigned int min;
            unsigned int max;
        } uint_opt;
        struct {
            const char *const *values;
        } enum_opt;
        struct {
            const char *const *values;
        } flag_opt;
    } u;
    // Optional
    void (*on_change)(bool global);
} OptionDesc;

typedef union {
    // OPT_STR
    const char *str_val;
    // OPT_UINT, OPT_ENUM, OPT_FLAG
    unsigned int uint_val;
    // OPT_BOOL
    bool bool_val;
} OptionValue;

#define STR_OPT(_name, OLG, _validate, _on_change) { \
    .name = _name, \
    .type = OPT_STR, \
    OLG \
    .u = { .str_opt = { \
        .validate = _validate, \
    } }, \
    .on_change = _on_change, \
}

#define UINT_OPT(_name, OLG, _min, _max, _on_change) { \
    .name = _name, \
    .type = OPT_UINT, \
    OLG \
    .u = { .uint_opt = { \
        .min = _min, \
        .max = _max, \
    } }, \
    .on_change = _on_change, \
}

#define ENUM_OPT(_name, OLG, _values, _on_change) { \
    .name = _name, \
    .type = OPT_ENUM, \
    OLG \
    .u = { .enum_opt = { \
        .values = _values, \
    } }, \
    .on_change = _on_change, \
}

#define FLAG_OPT(_name, OLG, _values, _on_change) { \
    .name = _name, \
    .type = OPT_FLAG, \
    OLG \
    .u = { .flag_opt = { \
        .values = _values, \
    } }, \
    .on_change = _on_change, \
}

#define BOOL_OPT(_name, OLG, _on_change) { \
    .name = _name, \
    .type = OPT_BOOL, \
    OLG \
    .u = { .enum_opt = { \
        .values = bool_enum, \
    } }, \
    .on_change = _on_change, \
}

#define OLG(_offset, _local, _global) \
    .offset = _offset, \
    .local = _local, \
    .global = _global, \

#define L(member) OLG(offsetof(LocalOptions, member), true, false)
#define G(member) OLG(offsetof(GlobalOptions, member), false, true)
#define C(member) OLG(offsetof(CommonOptions, member), true, true)

static void filetype_changed(bool global)
{
    BUG_ON(!buffer);
    BUG_ON(global);
    set_file_options(buffer);
    buffer_update_syntax(buffer);
}

static void set_window_title_changed(bool global)
{
    BUG_ON(!global);
    if (editor.options.set_window_title) {
        if (editor.status == EDITOR_RUNNING) {
            update_term_title(buffer);
        }
    } else {
        term_restore_title();
        term_save_title();
    }
}

static void syntax_changed(bool global)
{
    if (buffer && !global) {
        buffer_update_syntax(buffer);
    }
}

static void redraw_buffer(bool global)
{
    if (buffer && !global) {
        mark_all_lines_changed(buffer);
    }
}

static void redraw_screen(bool UNUSED_ARG(global))
{
    mark_everything_changed();
}

static bool validate_statusline_format(const char *value)
{
    static const StringView chars = STRING_VIEW("bfmryYxXpEMNSnstu%");
    size_t i = 0;
    while (value[i]) {
        char ch = value[i++];
        if (ch == '%') {
            ch = value[i++];
            if (ch == '\0') {
                error_msg("Format character expected after '%%'");
                return false;
            }
            if (!strview_memchr(&chars, ch)) {
                error_msg("Invalid format character '%c'", ch);
                return false;
            }
        }
    }
    return true;
}

static bool validate_filetype(const char *value)
{
    if (!is_ft(value)) {
        error_msg("No such file type %s", value);
        return false;
    }
    return true;
}

static bool validate_regex(const char *value)
{
    return value[0] == '\0' || regexp_is_valid(value, REG_NEWLINE);
}

static OptionValue str_get(const OptionDesc* UNUSED_ARG(desc), void *ptr)
{
    OptionValue v;
    const char **strp = ptr;
    v.str_val = *strp;
    return v;
}

static void str_set(const OptionDesc* UNUSED_ARG(d), void *ptr, OptionValue v)
{
    const char **strp = ptr;
    *strp = str_intern(v.str_val);
}

static bool str_parse(const OptionDesc *d, const char *str, OptionValue *v)
{
    if (d->u.str_opt.validate && !d->u.str_opt.validate(str)) {
        v->str_val = NULL;
        return false;
    }
    v->str_val = str;
    return true;
}

static const char *str_string(const OptionDesc* UNUSED_ARG(d), OptionValue v)
{
    const char *s = v.str_val;
    return s ? s : "";
}

static bool str_equals(const OptionDesc* UNUSED_ARG(d), void *ptr, OptionValue v)
{
    const char **strp = ptr;
    return xstreq(*strp, v.str_val);
}

static OptionValue uint_get(const OptionDesc* UNUSED_ARG(desc), void *ptr)
{
    OptionValue v;
    unsigned int *valp = ptr;
    v.uint_val = *valp;
    return v;
}

static void uint_set(const OptionDesc* UNUSED_ARG(d), void *ptr, OptionValue v)
{
    unsigned int *valp = ptr;
    *valp = v.uint_val;
}

static bool uint_parse(const OptionDesc *d, const char *str, OptionValue *v)
{
    unsigned int val;
    if (!str_to_uint(str, &val)) {
        error_msg("Integer value for %s expected", d->name);
        return false;
    }
    const unsigned int min = d->u.uint_opt.min;
    const unsigned int max = d->u.uint_opt.max;
    if (val < min || val > max) {
        error_msg("Value for %s must be in %u-%u range", d->name, min, max);
        return false;
    }
    v->uint_val = val;
    return true;
}

static const char *uint_string(const OptionDesc* UNUSED_ARG(desc), OptionValue value)
{
    return uint_to_str(value.uint_val);
}

static bool uint_equals(const OptionDesc* UNUSED_ARG(desc), void *ptr, OptionValue value)
{
    unsigned int *valp = ptr;
    return *valp == value.uint_val;
}

static OptionValue bool_get(const OptionDesc* UNUSED_ARG(d), void *ptr)
{
    OptionValue v;
    bool *valp = ptr;
    v.bool_val = *valp;
    return v;
}

static void bool_set(const OptionDesc* UNUSED_ARG(d), void *ptr, OptionValue v)
{
    bool *valp = ptr;
    *valp = v.bool_val;
}

static bool bool_parse(const OptionDesc *d, const char *str, OptionValue *v)
{
    if (streq(str, "true")) {
        v->bool_val = true;
        return true;
    } else if (streq(str, "false")) {
        v->bool_val = false;
        return true;
    }
    error_msg("Invalid value for %s", d->name);
    return false;
}

static const char *bool_string(const OptionDesc* UNUSED_ARG(d), OptionValue v)
{
    return v.bool_val ? "true" : "false";
}

static bool bool_equals(const OptionDesc* UNUSED_ARG(d), void *ptr, OptionValue v)
{
    bool *valp = ptr;
    return *valp == v.bool_val;
}

static bool enum_parse(const OptionDesc *d, const char *str, OptionValue *v)
{
    const char *const *values = d->u.enum_opt.values;
    unsigned int i;
    for (i = 0; values[i]; i++) {
        if (streq(values[i], str)) {
            v->uint_val = i;
            return true;
        }
    }
    unsigned int val;
    if (!str_to_uint(str, &val) || val >= i) {
        error_msg("Invalid value for %s", d->name);
        return false;
    }
    v->uint_val = val;
    return true;
}

static const char *enum_string(const OptionDesc *desc, OptionValue value)
{
    return desc->u.enum_opt.values[value.uint_val];
}

static bool flag_parse(const OptionDesc *d, const char *str, OptionValue *v)
{
    // "0" is allowed for compatibility and is the same as ""
    if (str[0] == '0' && str[1] == '\0') {
        v->uint_val = 0;
        return true;
    }

    const char *const *values = d->u.flag_opt.values;
    unsigned int flags = 0;

    for (size_t pos = 0, len = strlen(str); pos < len; ) {
        const StringView flag = get_delim(str, &pos, len, ',');
        size_t i;
        for (i = 0; values[i]; i++) {
            if (strview_equal_cstring(&flag, values[i])) {
                flags |= 1u << i;
                break;
            }
        }
        if (unlikely(!values[i])) {
            int flen = (int)flag.length;
            error_msg("Invalid flag '%.*s' for %s", flen, flag.data, d->name);
            return false;
        }
    }

    v->uint_val = flags;
    return true;
}

static const char *flag_string(const OptionDesc *desc, OptionValue value)
{
    static char buf[256];
    unsigned int flags = value.uint_val;
    if (!flags) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    char *ptr = buf;
    const char *const *values = desc->u.flag_opt.values;
    for (size_t i = 0; values[i]; i++) {
        if (flags & (1u << i)) {
            size_t len = strlen(values[i]);
            memcpy(ptr, values[i], len);
            ptr += len;
            *ptr++ = ',';
        }
    }

    ptr[-1] = '\0';
    return buf;
}

static const struct {
    OptionValue (*get)(const OptionDesc *desc, void *ptr);
    void (*set)(const OptionDesc *desc, void *ptr, OptionValue value);
    bool (*parse)(const OptionDesc *desc, const char *str, OptionValue *value);
    const char *(*string)(const OptionDesc *desc, OptionValue value);
    bool (*equals)(const OptionDesc *desc, void *ptr, OptionValue value);
} option_ops[] = {
    [OPT_STR] = {str_get, str_set, str_parse, str_string, str_equals},
    [OPT_UINT] = {uint_get, uint_set, uint_parse, uint_string, uint_equals},
    [OPT_ENUM] = {uint_get, uint_set, enum_parse, enum_string, uint_equals},
    [OPT_BOOL] = {bool_get, bool_set, bool_parse, bool_string, bool_equals},
    [OPT_FLAG] = {uint_get, uint_set, flag_parse, flag_string, uint_equals},
};

static const char *const bool_enum[] = {"false", "true", NULL};
static const char *const newline_enum[] = {"unix", "dos", NULL};
static const char *const tristate_enum[] = {"false", "true", "auto", NULL};

static const char *const detect_indent_values[] = {
    "1", "2", "3", "4", "5", "6", "7", "8",
    NULL
};

static const char *const ws_error_values[] = {
    "space-indent",
    "space-align",
    "tab-indent",
    "tab-after-indent",
    "special",
    "auto-indent",
    "trailing",
    "all-trailing",
    NULL
};

static const OptionDesc option_desc[] = {
    BOOL_OPT("auto-indent", C(auto_indent), NULL),
    BOOL_OPT("brace-indent", L(brace_indent), NULL),
    ENUM_OPT("case-sensitive-search", G(case_sensitive_search), tristate_enum, NULL),
    FLAG_OPT("detect-indent", C(detect_indent), detect_indent_values, NULL),
    BOOL_OPT("display-invisible", G(display_invisible), redraw_screen),
    BOOL_OPT("display-special", G(display_special), redraw_screen),
    BOOL_OPT("editorconfig", C(editorconfig), NULL),
    BOOL_OPT("emulate-tab", C(emulate_tab), NULL),
    UINT_OPT("esc-timeout", G(esc_timeout), 0, 2000, NULL),
    BOOL_OPT("expand-tab", C(expand_tab), redraw_buffer),
    BOOL_OPT("file-history", C(file_history), NULL),
    UINT_OPT("filesize-limit", G(filesize_limit), 0, 16000, NULL),
    STR_OPT("filetype", L(filetype), validate_filetype, filetype_changed),
    BOOL_OPT("fsync", C(fsync), NULL),
    STR_OPT("indent-regex", L(indent_regex), validate_regex, NULL),
    UINT_OPT("indent-width", C(indent_width), 1, 8, NULL),
    BOOL_OPT("lock-files", G(lock_files), NULL),
    ENUM_OPT("newline", G(crlf_newlines), newline_enum, NULL),
    UINT_OPT("scroll-margin", G(scroll_margin), 0, 100, redraw_screen),
    BOOL_OPT("select-cursor-char", G(select_cursor_char), redraw_screen),
    BOOL_OPT("set-window-title", G(set_window_title), set_window_title_changed),
    BOOL_OPT("show-line-numbers", G(show_line_numbers), redraw_screen),
    STR_OPT("statusline-left", G(statusline_left), validate_statusline_format, NULL),
    STR_OPT("statusline-right", G(statusline_right), validate_statusline_format, NULL),
    BOOL_OPT("syntax", C(syntax), syntax_changed),
    BOOL_OPT("tab-bar", G(tab_bar), redraw_screen),
    UINT_OPT("tab-width", C(tab_width), 1, 8, redraw_buffer),
    UINT_OPT("text-width", C(text_width), 1, 1000, NULL),
    BOOL_OPT("utf8-bom", G(utf8_bom), NULL),
    FLAG_OPT("ws-error", C(ws_error), ws_error_values, redraw_buffer),
};

static char *local_ptr(const OptionDesc *desc, const LocalOptions *opt)
{
    return (char*)opt + desc->offset;
}

static char *global_ptr(const OptionDesc *desc)
{
    return (char*)&editor.options + desc->offset;
}

UNITTEST {
    static const size_t alignments[] = {
        [OPT_STR] = alignof(const char*),
        [OPT_UINT] = alignof(unsigned int),
        [OPT_ENUM] = alignof(unsigned int),
        [OPT_BOOL] = alignof(bool),
        [OPT_FLAG] = alignof(unsigned int),
    };

    // Check offset alignments
    for (size_t i = 0; i < ARRAY_COUNT(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        size_t alignment = alignments[desc->type];
        if (desc->global) {
            uintptr_t ptr_val = (uintptr_t)global_ptr(desc);
            BUG_ON(ptr_val % alignment != 0);
        }
        if (desc->local) {
            const UNUSED Buffer b;
            uintptr_t ptr_val = (uintptr_t)local_ptr(desc, &b.options);
            BUG_ON(ptr_val % alignment != 0);
        }
    }

    // Ensure option_desc[] is properly sorted
    CHECK_BSEARCH_ARRAY(option_desc, name, strcmp);

    // Validate default statusline formats
    BUG_ON(!validate_statusline_format(editor.options.statusline_left));
    BUG_ON(!validate_statusline_format(editor.options.statusline_right));
}

static OptionValue desc_get(const OptionDesc *desc, void *ptr)
{
    return option_ops[desc->type].get(desc, ptr);
}

static void desc_set(const OptionDesc *desc, void *ptr, bool global, OptionValue value)
{
    option_ops[desc->type].set(desc, ptr, value);
    if (desc->on_change) {
        desc->on_change(global);
    }
}

static bool desc_parse(const OptionDesc *desc, const char *str, OptionValue *value)
{
    return option_ops[desc->type].parse(desc, str, value);
}

static const char *desc_string(const OptionDesc *desc, OptionValue value)
{
    return option_ops[desc->type].string(desc, value);
}

static bool desc_equals(const OptionDesc *desc, void *ptr, OptionValue value)
{
    return option_ops[desc->type].equals(desc, ptr, value);
}

static int option_cmp(const void *key, const void *elem)
{
    const char *name = key;
    const OptionDesc *desc = elem;
    return strcmp(name, desc->name);
}

static const OptionDesc *find_option(const char *name)
{
    return BSEARCH(name, option_desc, option_cmp);
}

static const OptionDesc *must_find_option(const char *name)
{
    const OptionDesc *desc = find_option(name);
    if (!desc) {
        error_msg("No such option %s", name);
    }
    return desc;
}

static const OptionDesc *must_find_global_option(const char *name)
{
    const OptionDesc *desc = must_find_option(name);
    if (desc && !desc->global) {
        error_msg("Option %s is not global", name);
        return NULL;
    }
    return desc;
}

static void do_set_option (
    const OptionDesc *desc,
    const char *value,
    bool local,
    bool global
) {
    if (local && !desc->local) {
        error_msg("Option %s is not local", desc->name);
        return;
    }
    if (global && !desc->global) {
        error_msg("Option %s is not global", desc->name);
        return;
    }

    OptionValue val;
    if (!desc_parse(desc, value, &val)) {
        return;
    }
    if (!local && !global) {
        // Set both by default
        if (desc->local) {
            local = true;
        }
        if (desc->global) {
            global = true;
        }
    }

    if (local) {
        desc_set(desc, local_ptr(desc, &buffer->options), false, val);
    }
    if (global) {
        desc_set(desc, global_ptr(desc), true, val);
    }
}

void set_option(const char *name, const char *value, bool local, bool global)
{
    const OptionDesc *desc = must_find_option(name);
    if (!desc) {
        return;
    }
    do_set_option(desc, value, local, global);
}

void set_bool_option(const char *name, bool local, bool global)
{
    const OptionDesc *desc = must_find_option(name);
    if (!desc) {
        return;
    }
    if (desc->type != OPT_BOOL) {
        error_msg("Option %s is not boolean", desc->name);
        return;
    }
    do_set_option(desc, "true", local, global);
}

static const OptionDesc *find_toggle_option(const char *name, bool *global)
{
    if (*global) {
        return must_find_global_option(name);
    }

    // Toggle local value by default if option has both values
    const OptionDesc *desc = must_find_option(name);
    if (desc && !desc->local) {
        *global = true;
    }
    return desc;
}

void toggle_option(const char *name, bool global, bool verbose)
{
    const OptionDesc *desc = find_toggle_option(name, &global);
    if (!desc) {
        return;
    }

    char *ptr = global ? global_ptr(desc) : local_ptr(desc, &buffer->options);
    OptionValue value = desc_get(desc, ptr);
    OptionType type = desc->type;
    if (type == OPT_ENUM) {
        if (desc->u.enum_opt.values[value.uint_val + 1]) {
            value.uint_val++;
        } else {
            value.uint_val = 0;
        }
    } else if (type == OPT_BOOL) {
        value.bool_val = !value.bool_val;
    } else {
        error_msg("Toggling %s requires arguments", name);
        return;
    }

    desc_set(desc, ptr, global, value);
    if (verbose) {
        const char *str = desc_string(desc, value);
        info_msg("%s = %s", desc->name, str);
    }
}

void toggle_option_values (
    const char *name,
    bool global,
    bool verbose,
    char **values,
    size_t count
) {
    const OptionDesc *desc = find_toggle_option(name, &global);
    if (!desc) {
        return;
    }

    BUG_ON(count == 0);
    size_t current = 0;
    bool error = false;
    char *ptr = global ? global_ptr(desc) : local_ptr(desc, &buffer->options);
    OptionValue *parsed_values = xnew(OptionValue, count);

    for (size_t i = 0; i < count; i++) {
        if (desc_parse(desc, values[i], &parsed_values[i])) {
            if (desc_equals(desc, ptr, parsed_values[i])) {
                current = i + 1;
            }
        } else {
            error = true;
        }
    }

    if (!error) {
        size_t i = current % count;
        desc_set(desc, ptr, global, parsed_values[i]);
        if (verbose) {
            const char *str = desc_string(desc, parsed_values[i]);
            info_msg("%s = %s", desc->name, str);
        }
    }

    free(parsed_values);
}

bool validate_local_options(char **strs)
{
    size_t invalid = 0;
    for (size_t i = 0; strs[i]; i += 2) {
        const char *name = strs[i];
        const char *value = strs[i + 1];
        const OptionDesc *desc = must_find_option(name);
        if (unlikely(!desc)) {
            invalid++;
        } else if (unlikely(!desc->local)) {
            error_msg("%s is not local", name);
            invalid++;
        } else if (unlikely(desc->on_change == filetype_changed)) {
            error_msg("filetype cannot be set via option command");
            invalid++;
        } else {
            OptionValue val;
            if (unlikely(!desc_parse(desc, value, &val))) {
                invalid++;
            }
        }
    }
    return !invalid;
}

void collect_options(const char *prefix, bool local, bool global)
{
    for (size_t i = 0; i < ARRAY_COUNT(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        if ((local && !desc->local) || (global && !desc->global)) {
            continue;
        }
        if (str_has_prefix(desc->name, prefix)) {
            add_completion(xstrdup(desc->name));
        }
    }
}

// Collect options that can be set via the "option" command
void collect_auto_options(const char *prefix)
{
    for (size_t i = 0; i < ARRAY_COUNT(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        if (!desc->local || desc->on_change == filetype_changed) {
            continue;
        }
        if (str_has_prefix(desc->name, prefix)) {
            add_completion(xstrdup(desc->name));
        }
    }
}

void collect_toggleable_options(const char *prefix, bool global)
{
    for (size_t i = 0; i < ARRAY_COUNT(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        if (global && !desc->global) {
            continue;
        }
        OptionType type = desc->type;
        bool toggleable = (type == OPT_ENUM || type == OPT_BOOL);
        if (toggleable && str_has_prefix(desc->name, prefix)) {
            add_completion(xstrdup(desc->name));
        }
    }
}

void collect_option_values(const char *option, const char *prefix)
{
    const OptionDesc *desc = find_option(option);
    if (!desc) {
        return;
    }

    if (prefix[0] == '\0') {
        bool local = desc->local;
        char *ptr = local ? local_ptr(desc, &buffer->options) : global_ptr(desc);
        OptionValue value = desc_get(desc, ptr);
        add_completion(xstrdup(desc_string(desc, value)));
        return;
    }

    const char *const *values;
    const char *comma;
    size_t prefix_len;

    switch (desc->type) {
    case OPT_ENUM:
    case OPT_BOOL:
        values = desc->u.enum_opt.values;
        for (size_t i = 0; values[i]; i++) {
            if (str_has_prefix(values[i], prefix)) {
                add_completion(xstrdup(values[i]));
            }
        }
        break;
    case OPT_FLAG:
        values = desc->u.flag_opt.values;
        comma = strrchr(prefix, ',');
        prefix_len = comma ? ++comma - prefix : 0;
        for (size_t i = 0; values[i]; i++) {
            const char *str = values[i];
            if (str_has_prefix(str, prefix + prefix_len)) {
                size_t str_len = strlen(str);
                char *completion = xmalloc(prefix_len + str_len + 1);
                memcpy(completion, prefix, prefix_len);
                memcpy(completion + prefix_len, str, str_len + 1);
                add_completion(completion);
            }
        }
        break;
    case OPT_STR:
    case OPT_UINT:
        break;
    default:
        BUG("unhandled option type");
    }
}

static void append_option(String *s, const OptionDesc *desc, void *ptr)
{
    const OptionValue value = desc_get(desc, ptr);
    const char *value_str = desc_string(desc, value);
    if (unlikely(value_str[0] == '-')) {
        string_append_literal(s, "-- ");
    }
    string_append_cstring(s, desc->name);
    string_append_byte(s, ' ');
    string_append_escaped_arg(s, value_str, true);
    string_append_byte(s, '\n');
}

String dump_options(void)
{
    String buf = string_new(4096);
    for (size_t i = 0; i < ARRAY_COUNT(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        void *local = desc->local ? local_ptr(desc, &buffer->options) : NULL;
        void *global = desc->global ? global_ptr(desc) : NULL;
        if (local && global) {
            const OptionValue global_value = desc_get(desc, global);
            if (desc_equals(desc, local, global_value)) {
                string_append_literal(&buf, "set ");
                append_option(&buf, desc, local);
            } else {
                string_append_literal(&buf, "set -g ");
                append_option(&buf, desc, global);
                string_append_literal(&buf, "set -l ");
                append_option(&buf, desc, local);
            }
        } else {
            string_append_literal(&buf, "set ");
            append_option(&buf, desc, local ? local : global);
        }
    }
    string_append_literal(&buf, "\n\n");
    dump_file_options(&buf);
    return buf;
}

const char *get_option_value_string(const char *name)
{
    const OptionDesc *desc = find_option(name);
    if (!desc) {
        return NULL;
    }
    bool local = desc->local;
    char *ptr = local ? local_ptr(desc, &buffer->options) : global_ptr(desc);
    return desc_string(desc, desc_get(desc, ptr));
}
