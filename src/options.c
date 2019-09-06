#include "options.h"
#include "completion.h"
#include "debug.h"
#include "editor.h"
#include "error.h"
#include "file-option.h"
#include "filetype.h"
#include "regexp.h"
#include "screen.h"
#include "terminal/terminal.h"
#include "util/hashset.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "view.h"
#include "window.h"

typedef enum {
    OPT_STR,
    OPT_UINT,
    OPT_ENUM,
    OPT_FLAG,
} OptionType;

typedef struct {
    const struct OptionOps *ops;
    const char *name;
    size_t offset;
    bool local;
    bool global;
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
            const char **values;
        } enum_opt;
        struct {
            const char **values;
        } flag_opt;
    } u;
    // Optional
    void (*on_change)(void);
} OptionDesc;

typedef union {
    // OPT_STR
    const char *str_val;
    // OPT_UINT, OPT_ENUM, OPT_FLAG
    unsigned int uint_val;
} OptionValue;

typedef struct OptionOps {
    OptionValue (*get)(const OptionDesc *desc, void *ptr);
    void (*set)(const OptionDesc *desc, void *ptr, OptionValue value);
    bool (*parse)(const OptionDesc *desc, const char *str, OptionValue *value);
    const char *(*string)(const OptionDesc *desc, OptionValue value);
    bool (*equals)(const OptionDesc *desc, void *ptr, OptionValue value);
} OptionOps;

#define STR_OPT(_name, OLG, _validate, _on_change) { \
    .ops = &option_ops[OPT_STR], \
    .name = _name, \
    OLG \
    .u = { .str_opt = { \
        .validate = _validate, \
    } }, \
    .on_change = _on_change, \
}

#define UINT_OPT(_name, OLG, _min, _max, _on_change) { \
    .ops = &option_ops[OPT_UINT], \
    .name = _name, \
    OLG \
    .u = { .uint_opt = { \
        .min = _min, \
        .max = _max, \
    } }, \
    .on_change = _on_change, \
}

#define ENUM_OPT(_name, OLG, _values, _on_change) { \
    .ops = &option_ops[OPT_ENUM], \
    .name = _name, \
    OLG \
    .u = { .enum_opt = { \
        .values = _values, \
    } }, \
    .on_change = _on_change, \
}

#define FLAG_OPT(_name, OLG, _values, _on_change) { \
    .ops = &option_ops[OPT_FLAG], \
    .name = _name, \
    OLG \
    .u = { .flag_opt = { \
        .values = _values, \
    } }, \
    .on_change = _on_change, \
}

// Can't reuse ENUM_OPT() because of weird macro expansion rules
#define BOOL_OPT(_name, OLG, _on_change) { \
    .ops = &option_ops[OPT_ENUM], \
    .name = _name, \
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

static void filetype_changed(void)
{
    Buffer *b = window->view->buffer;
    set_file_options(b);
    buffer_update_syntax(b);
}

static void set_window_title_changed(void)
{
    if (editor.options.set_window_title) {
        if (editor.status == EDITOR_RUNNING) {
            update_term_title(window->view->buffer);
        }
    } else {
        terminal.restore_title();
        terminal.save_title();
    }
}

static void syntax_changed(void)
{
    if (window->view != NULL) {
        buffer_update_syntax(window->view->buffer);
    }
}

static bool validate_statusline_format(const char *value)
{
    static const char chars[] = "fmryYxXpEMnstu%";
    size_t i = 0;
    while (value[i]) {
        char ch = value[i++];
        if (ch == '%') {
            ch = value[i++];
            if (!ch) {
                error_msg("Format character expected after '%%'.");
                return false;
            }
            if (!strchr(chars, ch)) {
                error_msg("Invalid format character '%c'.", ch);
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
    v.str_val = *(char**)ptr;
    return v;
}

static void str_set (
    const OptionDesc* UNUSED_ARG(desc),
    void *ptr,
    OptionValue value
) {
    const char **strp = ptr;
    *strp = str_intern(value.str_val);
}

static bool str_parse (
    const OptionDesc *desc,
    const char *str,
    OptionValue *value
) {
    if (desc->u.str_opt.validate && !desc->u.str_opt.validate(str)) {
        value->str_val = NULL;
        return false;
    }
    value->str_val = str;
    return true;
}

static const char *str_string(const OptionDesc* UNUSED_ARG(desc), OptionValue value)
{
    const char *s = value.str_val;
    return s ? s : "";
}

static bool str_equals (
    const OptionDesc* UNUSED_ARG(desc),
    void *ptr,
    OptionValue value
) {
    return xstreq(*(char**)ptr, value.str_val);
}

static OptionValue uint_get(const OptionDesc* UNUSED_ARG(desc), void *ptr)
{
    OptionValue v;
    v.uint_val = *(unsigned int*)ptr;
    return v;
}

static void uint_set (
    const OptionDesc* UNUSED_ARG(desc),
    void *ptr,
    OptionValue value
) {
    *(unsigned int*)ptr = value.uint_val;
}

static bool uint_parse (
    const OptionDesc *desc,
    const char *str,
    OptionValue *value
) {
    unsigned int val;
    if (!str_to_uint(str, &val)) {
        error_msg("Integer value for %s expected.", desc->name);
        return false;
    }
    if (val < desc->u.uint_opt.min || val > desc->u.uint_opt.max) {
        error_msg (
            "Value for %s must be in %u-%u range.",
            desc->name,
            desc->u.uint_opt.min,
            desc->u.uint_opt.max
        );
        return false;
    }
    value->uint_val = val;
    return true;
}

static const char *uint_string(const OptionDesc* UNUSED_ARG(desc), OptionValue value)
{
    static char buf[64];
    xsnprintf(buf, sizeof buf, "%u", value.uint_val);
    return buf;
}

static bool uint_equals(const OptionDesc* UNUSED_ARG(desc), void *ptr, OptionValue value)
{
    return *(unsigned int*)ptr == value.uint_val;
}

static bool enum_parse (
    const OptionDesc *desc,
    const char *str,
    OptionValue *value
) {
    unsigned int i;
    for (i = 0; desc->u.enum_opt.values[i]; i++) {
        if (streq(desc->u.enum_opt.values[i], str)) {
            value->uint_val = i;
            return true;
        }
    }
    unsigned int val;
    if (!str_to_uint(str, &val) || val >= i) {
        error_msg("Invalid value for %s.", desc->name);
        return false;
    }
    value->uint_val = val;
    return true;
}

static const char *enum_string(const OptionDesc *desc, OptionValue value)
{
    return desc->u.enum_opt.values[value.uint_val];
}

static bool flag_parse (
    const OptionDesc *desc,
    const char *str,
    OptionValue *value
) {
    // "0" is allowed for compatibility and is the same as ""
    if (str[0] == '0' && str[1] == '\0') {
        value->uint_val = 0;
        return true;
    }

    const char **values = desc->u.flag_opt.values;
    const char *ptr = str;
    unsigned int flags = 0;

    while (*ptr) {
        const char *end = strchr(ptr, ',');
        size_t len;
        if (end) {
            len = end - ptr;
            end++;
        } else {
            len = strlen(ptr);
            end = ptr + len;
        }
        const StringView flag = string_view(ptr, len);
        ptr = end;
        size_t i;
        for (i = 0; values[i]; i++) {
            if (string_view_equal_cstr(&flag, values[i])) {
                flags |= 1u << i;
                break;
            }
        }
        if (!values[i]) {
            error_msg (
                "Invalid flag '%.*s' for %s.",
                (int)flag.length,
                flag.data,
                desc->name
            );
            return false;
        }
    }
    value->uint_val = flags;
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
    const char **values = desc->u.flag_opt.values;
    for (size_t i = 0; values[i]; i++) {
        if (flags & (1 << i)) {
            size_t len = strlen(values[i]);
            memcpy(ptr, values[i], len);
            ptr += len;
            *ptr++ = ',';
        }
    }

    ptr[-1] = '\0';
    return buf;
}

static const OptionOps option_ops[] = {
    [OPT_STR] = {str_get, str_set, str_parse, str_string, str_equals},
    [OPT_UINT] = {uint_get, uint_set, uint_parse, uint_string, uint_equals},
    [OPT_ENUM] = {uint_get, uint_set, enum_parse, enum_string, uint_equals},
    [OPT_FLAG] = {uint_get, uint_set, flag_parse, flag_string, uint_equals},
};

static const char *bool_enum[] = {"false", "true", NULL};
static const char *newline_enum[] = {"unix", "dos", NULL};

static const char *case_sensitive_search_enum[] = {
    "false",
    "true",
    "auto",
    NULL
};

static const char *detect_indent_values[] = {
    "1", "2", "3", "4", "5", "6", "7", "8",
    NULL
};

static const char *tab_bar_enum[] = {
    "hidden",
    "horizontal",
    "vertical",
    "auto",
    NULL
};

static const char *ws_error_values[] = {
    "trailing",
    "space-indent",
    "space-align",
    "tab-indent",
    "tab-after-indent",
    "special",
    "auto-indent",
    NULL
};

static const OptionDesc option_desc[] = {
    BOOL_OPT("auto-indent", C(auto_indent), NULL),
    BOOL_OPT("brace-indent", L(brace_indent), NULL),
    ENUM_OPT("case-sensitive-search", G(case_sensitive_search), case_sensitive_search_enum, NULL),
    FLAG_OPT("detect-indent", C(detect_indent), detect_indent_values, NULL),
    BOOL_OPT("display-invisible", G(display_invisible), NULL),
    BOOL_OPT("display-special", G(display_special), NULL),
    BOOL_OPT("editorconfig", C(editorconfig), NULL),
    BOOL_OPT("emulate-tab", C(emulate_tab), NULL),
    UINT_OPT("esc-timeout", G(esc_timeout), 0, 2000, NULL),
    BOOL_OPT("expand-tab", C(expand_tab), NULL),
    BOOL_OPT("file-history", C(file_history), NULL),
    UINT_OPT("filesize-limit", G(filesize_limit), 0, 16000, NULL),
    STR_OPT("filetype", L(filetype), validate_filetype, filetype_changed),
    UINT_OPT("indent-width", C(indent_width), 1, 8, NULL),
    STR_OPT("indent-regex", L(indent_regex), validate_regex, NULL),
    BOOL_OPT("lock-files", G(lock_files), NULL),
    ENUM_OPT("newline", G(newline), newline_enum, NULL),
    UINT_OPT("scroll-margin", G(scroll_margin), 0, 100, NULL),
    BOOL_OPT("set-window-title", G(set_window_title), set_window_title_changed),
    BOOL_OPT("show-line-numbers", G(show_line_numbers), NULL),
    STR_OPT("statusline-left", G(statusline_left), validate_statusline_format, NULL),
    STR_OPT("statusline-right", G(statusline_right), validate_statusline_format, NULL),
    BOOL_OPT("syntax", C(syntax), syntax_changed),
    ENUM_OPT("tab-bar", G(tab_bar), tab_bar_enum, NULL),
    UINT_OPT("tab-bar-max-components", G(tab_bar_max_components), 0, 10, NULL),
    UINT_OPT("tab-bar-width", G(tab_bar_width), TAB_BAR_MIN_WIDTH, 100, NULL),
    UINT_OPT("tab-width", C(tab_width), 1, 8, NULL),
    UINT_OPT("text-width", C(text_width), 1, 1000, NULL),
    FLAG_OPT("ws-error", C(ws_error), ws_error_values, NULL),
};

static char *local_ptr(const OptionDesc *desc, const LocalOptions *opt)
{
    return (char*)opt + desc->offset;
}

static char *global_ptr(const OptionDesc *desc)
{
    return (char*)&editor.options + desc->offset;
}

static bool desc_is(const OptionDesc *desc, OptionType type)
{
    return desc->ops == &option_ops[type];
}

static void desc_set(const OptionDesc *desc, void *ptr, OptionValue value)
{
    desc->ops->set(desc, ptr, value);
    if (desc->on_change) {
        desc->on_change();
    } else {
        mark_everything_changed();
    }
}

static const OptionDesc *find_option(const char *name)
{
    for (size_t i = 0; i < ARRAY_COUNT(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        if (streq(name, desc->name)) {
            return desc;
        }
    }
    return NULL;
}

static const OptionDesc *must_find_option(const char *name)
{
    const OptionDesc *desc = find_option(name);
    if (desc == NULL) {
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
    if (!desc->ops->parse(desc, value, &val)) {
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
        desc_set(desc, local_ptr(desc, &buffer->options), val);
    }
    if (global) {
        desc_set(desc, global_ptr(desc), val);
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
    if (!desc_is(desc, OPT_ENUM) || desc->u.enum_opt.values != bool_enum) {
        error_msg("Option %s is not boolean.", desc->name);
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

static unsigned int toggle(unsigned int value, const char **values)
{
    if (!values[++value]) {
        value = 0;
    }
    return value;
}

void toggle_option(const char *name, bool global, bool verbose)
{
    const OptionDesc *desc = find_toggle_option(name, &global);
    if (!desc) {
        return;
    }
    if (!desc_is(desc, OPT_ENUM)) {
        error_msg("Option %s is not toggleable.", name);
        return;
    }

    char *ptr;
    if (global) {
        ptr = global_ptr(desc);
    } else {
        ptr = local_ptr(desc, &buffer->options);
    }

    OptionValue value;
    value.uint_val = toggle(*(unsigned int *)ptr, desc->u.enum_opt.values);
    desc_set(desc, ptr, value);

    if (verbose) {
        const char *str = desc->ops->string(desc, value);
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
        if (desc->ops->parse(desc, values[i], &parsed_values[i])) {
            if (desc->ops->equals(desc, ptr, parsed_values[i])) {
                current = i + 1;
            }
        } else {
            error = true;
        }
    }

    if (!error) {
        size_t i = current % count;
        desc_set(desc, ptr, parsed_values[i]);
        if (verbose) {
            const char *str = desc->ops->string(desc, parsed_values[i]);
            info_msg("%s = %s", desc->name, str);
        }
    }

    free(parsed_values);
}

bool validate_local_options(char **strs)
{
    bool valid = true;
    for (size_t i = 0; strs[i]; i += 2) {
        const char *name = strs[i];
        const char *value = strs[i + 1];
        const OptionDesc *desc = must_find_option(name);
        if (desc == NULL) {
            valid = false;
        } else if (!desc->local) {
            error_msg("Option %s is not local", name);
            valid = false;
        } else if (streq(name, "filetype")) {
            error_msg("filetype cannot be set via option command");
            valid = false;
        } else {
            OptionValue val;
            if (!desc->ops->parse(desc, value, &val)) {
                valid = false;
            }
        }
    }
    return valid;
}

void collect_options(const char *prefix)
{
    for (size_t i = 0; i < ARRAY_COUNT(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        if (str_has_prefix(desc->name, prefix)) {
            add_completion(xstrdup(desc->name));
        }
    }
}

void collect_toggleable_options(const char *prefix)
{
    for (size_t i = 0; i < ARRAY_COUNT(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        if (desc_is(desc, OPT_ENUM) && str_has_prefix(desc->name, prefix)) {
            add_completion(xstrdup(desc->name));
        }
    }
}

void collect_option_values(const char *name, const char *prefix)
{
    const OptionDesc *desc = find_option(name);
    if (!desc) {
        return;
    }

    if (!*prefix) {
        // Complete value
        char *ptr;
        if (desc->local) {
            ptr = local_ptr(desc, &buffer->options);
        } else {
            ptr = global_ptr(desc);
        }
        OptionValue value = desc->ops->get(desc, ptr);
        add_completion(xstrdup(desc->ops->string(desc, value)));
    } else if (desc_is(desc, OPT_ENUM)) {
        // Complete possible values
        for (size_t i = 0; desc->u.enum_opt.values[i]; i++) {
            if (str_has_prefix(desc->u.enum_opt.values[i], prefix)) {
                add_completion(xstrdup(desc->u.enum_opt.values[i]));
            }
        }
    } else if (desc_is(desc, OPT_FLAG)) {
        // Complete possible values
        const char *comma = strrchr(prefix, ',');
        size_t prefix_len = 0;
        if (comma) {
            prefix_len = ++comma - prefix;
        }
        for (size_t i = 0; desc->u.flag_opt.values[i]; i++) {
            const char *str = desc->u.flag_opt.values[i];
            if (str_has_prefix(str, prefix + prefix_len)) {
                size_t str_len = strlen(str);
                char *completion = xmalloc(prefix_len + str_len + 1);
                memcpy(completion, prefix, prefix_len);
                memcpy(completion + prefix_len, str, str_len + 1);
                add_completion(completion);
            }
        }
    }
}
