#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"
#include "buffer.h"
#include "command/serialize.h"
#include "editor.h"
#include "error.h"
#include "file-option.h"
#include "filetype.h"
#include "screen.h"
#include "status.h"
#include "terminal/output.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/numtostr.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"

typedef enum {
    OPT_STR,
    OPT_UINT,
    OPT_ENUM,
    OPT_BOOL,
    OPT_FLAG,
    OPT_REGEX,
} OptionType;

typedef union {
    const char *str_val; // OPT_STR, OPT_REGEX
    unsigned int uint_val; // OPT_UINT, OPT_ENUM, OPT_FLAG
    bool bool_val; // OPT_BOOL
} OptionValue;

typedef union {
    struct {bool (*validate)(const char *value);} str_opt; // OPT_STR (optional)
    struct {unsigned int min, max;} uint_opt; // OPT_UINT
    struct {const char *const *values;} enum_opt; // OPT_ENUM, OPT_FLAG, OPT_BOOL
} OptionConstraint;

typedef struct {
    const char name[22];
    bool local;
    bool global;
    unsigned int offset;
    OptionType type;
    OptionConstraint u;
    void (*on_change)(EditorState *e, bool global); // Optional
} OptionDesc;

#define STR_OPT(_name, OLG, _validate, _on_change) { \
    OLG \
    .name = _name, \
    .type = OPT_STR, \
    .u = {.str_opt = {.validate = _validate}}, \
    .on_change = _on_change, \
}

#define UINT_OPT(_name, OLG, _min, _max, _on_change) { \
    OLG \
    .name = _name, \
    .type = OPT_UINT, \
    .u = {.uint_opt = { \
        .min = _min, \
        .max = _max, \
    }}, \
    .on_change = _on_change, \
}

#define ENUM_OPT(_name, OLG, _values, _on_change) { \
    OLG \
    .name = _name, \
    .type = OPT_ENUM, \
    .u = {.enum_opt = {.values = _values}}, \
    .on_change = _on_change, \
}

#define FLAG_OPT(_name, OLG, _values, _on_change) { \
    OLG \
    .name = _name, \
    .type = OPT_FLAG, \
    .u = {.enum_opt = {.values = _values}}, \
    .on_change = _on_change, \
}

#define BOOL_OPT(_name, OLG, _on_change) { \
    OLG \
    .name = _name, \
    .type = OPT_BOOL, \
    .u = {.enum_opt = {.values = bool_enum}}, \
    .on_change = _on_change, \
}

#define REGEX_OPT(_name, OLG, _on_change) { \
    OLG \
    .name = _name, \
    .type = OPT_REGEX, \
    .on_change = _on_change, \
}

#define OLG(_offset, _local, _global) \
    .offset = _offset, \
    .local = _local, \
    .global = _global, \

#define L(member) OLG(offsetof(LocalOptions, member), true, false)
#define G(member) OLG(offsetof(GlobalOptions, member), false, true)
#define C(member) OLG(offsetof(CommonOptions, member), true, true)

static void filetype_changed(EditorState *e, bool global)
{
    BUG_ON(!e->buffer);
    BUG_ON(global);
    set_file_options(e, e->buffer);
    buffer_update_syntax(e, e->buffer);
}

static void set_window_title_changed(EditorState *e, bool global)
{
    BUG_ON(!global);
    Terminal *term = &e->terminal;
    if (e->options.set_window_title) {
        if (e->status == EDITOR_RUNNING) {
            update_term_title(term, e->buffer, e->options.set_window_title);
        }
    } else {
        term_restore_title(term);
        term_save_title(term);
    }
}

static void syntax_changed(EditorState *e, bool global)
{
    if (e->buffer && !global) {
        buffer_update_syntax(e, e->buffer);
    }
}

static void overwrite_changed(EditorState *e, bool global)
{
    if (!global) {
        e->cursor_style_changed = true;
    }
}

static void redraw_buffer(EditorState *e, bool global)
{
    if (e->buffer && !global) {
        mark_all_lines_changed(e->buffer);
    }
}

static void redraw_screen(EditorState *e, bool global)
{
    BUG_ON(!global);
    mark_everything_changed(e);
}

static bool validate_statusline_format(const char *value)
{
    size_t errpos = statusline_format_find_error(value);
    if (likely(errpos == 0)) {
        return true;
    }
    char ch = value[errpos];
    if (ch == '\0') {
        return error_msg("Format character expected after '%%'");
    }
    return error_msg("Invalid format character '%c'", ch);
}

static bool validate_filetype(const char *value)
{
    if (!is_valid_filetype_name(value)) {
        return error_msg("Invalid filetype name '%s'", value);
    }
    return true;
}

static OptionValue str_get(const OptionDesc* UNUSED_ARG(desc), void *ptr)
{
    const char *const *strp = ptr;
    return (OptionValue){.str_val = *strp};
}

static void str_set(const OptionDesc* UNUSED_ARG(d), void *ptr, OptionValue v)
{
    const char **strp = ptr;
    *strp = str_intern(v.str_val);
}

static bool str_parse(const OptionDesc *d, const char *str, OptionValue *v)
{
    bool valid = !d->u.str_opt.validate || d->u.str_opt.validate(str);
    v->str_val = valid ? str : NULL;
    return valid;
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

static OptionValue re_get(const OptionDesc* UNUSED_ARG(desc), void *ptr)
{
    const InternedRegexp *const *irp = ptr;
    return (OptionValue){.str_val = *irp ? (*irp)->str : NULL};
}

static void re_set(const OptionDesc* UNUSED_ARG(d), void *ptr, OptionValue v)
{
    const InternedRegexp **irp = ptr;
    *irp = v.str_val ? regexp_intern(v.str_val) : NULL;
}

static bool re_parse(const OptionDesc* UNUSED_ARG(d), const char *str, OptionValue *v)
{
    if (str[0] == '\0') {
        v->str_val = NULL;
        return true;
    }

    bool valid = regexp_is_interned(str) || regexp_is_valid(str, REG_NEWLINE);
    v->str_val = valid ? str : NULL;
    return valid;
}

static bool re_equals(const OptionDesc* UNUSED_ARG(d), void *ptr, OptionValue v)
{
    const InternedRegexp **irp = ptr;
    return *irp ? xstreq((*irp)->str, v.str_val) : !v.str_val;
}

static OptionValue uint_get(const OptionDesc* UNUSED_ARG(desc), void *ptr)
{
    const unsigned int *valp = ptr;
    return (OptionValue){.uint_val = *valp};
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
        return error_msg("Integer value for %s expected", d->name);
    }

    const unsigned int min = d->u.uint_opt.min;
    const unsigned int max = d->u.uint_opt.max;
    if (val < min || val > max) {
        return error_msg("Value for %s must be in %u-%u range", d->name, min, max);
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
    const unsigned int *valp = ptr;
    return *valp == value.uint_val;
}

static OptionValue bool_get(const OptionDesc* UNUSED_ARG(d), void *ptr)
{
    const bool *valp = ptr;
    return (OptionValue){.bool_val = *valp};
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
    return error_msg("Invalid value for %s", d->name);
}

static const char *bool_string(const OptionDesc* UNUSED_ARG(d), OptionValue v)
{
    return v.bool_val ? "true" : "false";
}

static bool bool_equals(const OptionDesc* UNUSED_ARG(d), void *ptr, OptionValue v)
{
    const bool *valp = ptr;
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
        return error_msg("Invalid value for %s", d->name);
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

    const char *const *values = d->u.enum_opt.values;
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
            return error_msg("Invalid flag '%.*s' for %s", flen, flag.data, d->name);
        }
    }

    v->uint_val = flags;
    return true;
}

static const char *flag_string(const OptionDesc *desc, OptionValue value)
{
    static char buf[128];
    unsigned int flags = value.uint_val;
    if (!flags) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    char *ptr = buf;
    const char *const *values = desc->u.enum_opt.values;
    for (size_t i = 0, avail = sizeof(buf); values[i]; i++) {
        if (flags & (1u << i)) {
            char *next = memccpy(ptr, values[i], '\0', avail);
            if (DEBUG >= 1) {
                BUG_ON(!next);
                avail -= (size_t)(next - ptr);
            }
            ptr = next;
            ptr[-1] = ',';
        }
    }

    BUG_ON(ptr == buf);
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
    [OPT_REGEX] = {re_get, re_set, re_parse, str_string, re_equals},
};

static const char *const bool_enum[] = {"false", "true", NULL};
static const char *const newline_enum[] = {"unix", "dos", NULL};
static const char *const tristate_enum[] = {"false", "true", "auto", NULL};
static const char *const save_unmodified_enum[] = {"none", "touch", "full", NULL};

static const char *const detect_indent_values[] = {
    "1", "2", "3", "4", "5", "6", "7", "8",
    NULL
};

// Note: this must be kept in sync with WhitespaceErrorFlags
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
    BOOL_OPT("display-special", G(display_special), redraw_screen),
    BOOL_OPT("editorconfig", C(editorconfig), NULL),
    BOOL_OPT("emulate-tab", C(emulate_tab), NULL),
    UINT_OPT("esc-timeout", G(esc_timeout), 0, 2000, NULL),
    BOOL_OPT("expand-tab", C(expand_tab), redraw_buffer),
    BOOL_OPT("file-history", C(file_history), NULL),
    UINT_OPT("filesize-limit", G(filesize_limit), 0, 16000, NULL),
    STR_OPT("filetype", L(filetype), validate_filetype, filetype_changed),
    BOOL_OPT("fsync", C(fsync), NULL),
    REGEX_OPT("indent-regex", L(indent_regex), NULL),
    UINT_OPT("indent-width", C(indent_width), 1, INDENT_WIDTH_MAX, NULL),
    BOOL_OPT("lock-files", G(lock_files), NULL),
    ENUM_OPT("newline", G(crlf_newlines), newline_enum, NULL),
    BOOL_OPT("optimize-true-color", G(optimize_true_color), redraw_screen),
    BOOL_OPT("overwrite", C(overwrite), overwrite_changed),
    ENUM_OPT("save-unmodified", C(save_unmodified), save_unmodified_enum, NULL),
    UINT_OPT("scroll-margin", G(scroll_margin), 0, 100, redraw_screen),
    BOOL_OPT("select-cursor-char", G(select_cursor_char), redraw_screen),
    BOOL_OPT("set-window-title", G(set_window_title), set_window_title_changed),
    BOOL_OPT("show-line-numbers", G(show_line_numbers), redraw_screen),
    STR_OPT("statusline-left", G(statusline_left), validate_statusline_format, NULL),
    STR_OPT("statusline-right", G(statusline_right), validate_statusline_format, NULL),
    BOOL_OPT("syntax", C(syntax), syntax_changed),
    BOOL_OPT("tab-bar", G(tab_bar), redraw_screen),
    UINT_OPT("tab-width", C(tab_width), 1, TAB_WIDTH_MAX, redraw_buffer),
    UINT_OPT("text-width", C(text_width), 1, TEXT_WIDTH_MAX, NULL),
    BOOL_OPT("utf8-bom", G(utf8_bom), NULL),
    FLAG_OPT("ws-error", C(ws_error), ws_error_values, redraw_buffer),
};

static char *local_ptr(const OptionDesc *desc, LocalOptions *opt)
{
    BUG_ON(!desc->local);
    return (char*)opt + desc->offset;
}

static char *global_ptr(const OptionDesc *desc, GlobalOptions *opt)
{
    BUG_ON(!desc->global);
    return (char*)opt + desc->offset;
}

static char *get_option_ptr(EditorState *e, const OptionDesc *d, bool global)
{
    return global ? global_ptr(d, &e->options) : local_ptr(d, &e->buffer->options);
}

static inline size_t count_enum_values(const OptionDesc *desc)
{
    OptionType type = desc->type;
    BUG_ON(type != OPT_ENUM && type != OPT_FLAG && type != OPT_BOOL);
    const char *const *values = desc->u.enum_opt.values;
    BUG_ON(!values);
    return string_array_length((char**)values);
}

UNITTEST {
    static const struct {
        size_t alignment;
        size_t size;
    } map[] = {
        [OPT_STR] = {ALIGNOF(const char*), sizeof(const char*)},
        [OPT_UINT] = {ALIGNOF(unsigned int), sizeof(unsigned int)},
        [OPT_ENUM] = {ALIGNOF(unsigned int), sizeof(unsigned int)},
        [OPT_BOOL] = {ALIGNOF(bool), sizeof(bool)},
        [OPT_FLAG] = {ALIGNOF(unsigned int), sizeof(unsigned int)},
        [OPT_REGEX] = {ALIGNOF(const InternedRegexp*), sizeof(const InternedRegexp*)},
    };

    GlobalOptions gopts = {.tab_bar = true};
    LocalOptions lopts = {.filetype = NULL};

    for (size_t i = 0; i < ARRAYLEN(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        const OptionType type = desc->type;
        BUG_ON(type >= ARRAYLEN(map));
        size_t alignment = map[type].alignment;
        size_t end = desc->offset + map[type].size;
        if (desc->global) {
            uintptr_t ptr_val = (uintptr_t)global_ptr(desc, &gopts);
            BUG_ON(ptr_val % alignment != 0);
            BUG_ON(end > sizeof(GlobalOptions));
        }
        if (desc->local) {
            uintptr_t ptr_val = (uintptr_t)local_ptr(desc, &lopts);
            BUG_ON(ptr_val % alignment != 0);
            BUG_ON(end > sizeof(LocalOptions));
        }
        if (desc->global && desc->local) {
            BUG_ON(end > sizeof(CommonOptions));
        }
        if (type == OPT_UINT) {
            BUG_ON(desc->u.uint_opt.max <= desc->u.uint_opt.min);
        } else if (type == OPT_BOOL) {
            BUG_ON(desc->u.enum_opt.values != bool_enum);
        } else if (type == OPT_ENUM) {
            BUG_ON(count_enum_values(desc) < 2);
        } else if (type == OPT_FLAG) {
            size_t nvals = count_enum_values(desc);
            OptionValue val = {.uint_val = -1};
            BUG_ON(nvals < 2);
            BUG_ON(nvals >= BITSIZE(val.uint_val));
            const char *str = flag_string(desc, val);
            BUG_ON(!str);
            BUG_ON(str[0] == '\0');
            if (!flag_parse(desc, str, &val)) {
                BUG("flag_parse() failed for string: %s", str);
            }
            unsigned int mask = (1u << nvals) - 1;
            if (val.uint_val != mask) {
                BUG("values not equal: %u, %u", val.uint_val, mask);
            }
        }
    }

    // Ensure option_desc[] is properly sorted
    CHECK_BSEARCH_ARRAY(option_desc, name, strcmp);
}

static OptionValue desc_get(const OptionDesc *desc, void *ptr)
{
    return option_ops[desc->type].get(desc, ptr);
}

static void desc_set(EditorState *e, const OptionDesc *desc, void *ptr, bool global, OptionValue value)
{
    option_ops[desc->type].set(desc, ptr, value);
    if (desc->on_change) {
        desc->on_change(e, global);
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

static bool do_set_option (
    EditorState *e,
    const OptionDesc *desc,
    const char *value,
    bool local,
    bool global
) {
    if (local && !desc->local) {
        return error_msg("Option %s is not local", desc->name);
    }
    if (global && !desc->global) {
        return error_msg("Option %s is not global", desc->name);
    }

    OptionValue val;
    if (!desc_parse(desc, value, &val)) {
        return false;
    }

    if (!local && !global) {
        // Set both by default
        local = desc->local;
        global = desc->global;
    }

    if (local) {
        desc_set(e, desc, local_ptr(desc, &e->buffer->options), false, val);
    }
    if (global) {
        desc_set(e, desc, global_ptr(desc, &e->options), true, val);
    }

    return true;
}

bool set_option(EditorState *e, const char *name, const char *value, bool local, bool global)
{
    const OptionDesc *desc = must_find_option(name);
    if (!desc) {
        return false;
    }
    return do_set_option(e, desc, value, local, global);
}

bool set_bool_option(EditorState *e, const char *name, bool local, bool global)
{
    const OptionDesc *desc = must_find_option(name);
    if (!desc) {
        return false;
    }
    if (desc->type != OPT_BOOL) {
        return error_msg("Option %s is not boolean", desc->name);
    }
    return do_set_option(e, desc, "true", local, global);
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

bool toggle_option(EditorState *e, const char *name, bool global, bool verbose)
{
    const OptionDesc *desc = find_toggle_option(name, &global);
    if (!desc) {
        return false;
    }

    char *ptr = get_option_ptr(e, desc, global);
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
        return error_msg("Toggling %s requires arguments", name);
    }

    desc_set(e, desc, ptr, global, value);
    if (verbose) {
        const char *prefix = (global && desc->local) ? "[global] " : "";
        const char *str = desc_string(desc, value);
        info_msg("%s%s = %s", prefix, desc->name, str);
    }

    return true;
}

bool toggle_option_values (
    EditorState *e,
    const char *name,
    bool global,
    bool verbose,
    char **values,
    size_t count
) {
    const OptionDesc *desc = find_toggle_option(name, &global);
    if (!desc) {
        return false;
    }

    BUG_ON(count == 0);
    size_t current = 0;
    bool error = false;
    char *ptr = get_option_ptr(e, desc, global);
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
        desc_set(e, desc, ptr, global, parsed_values[i]);
        if (verbose) {
            const char *prefix = (global && desc->local) ? "[global] " : "";
            const char *str = desc_string(desc, parsed_values[i]);
            info_msg("%s%s = %s", prefix, desc->name, str);
        }
    }

    free(parsed_values);
    return !error;
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

#if DEBUG >= 1
static void sanity_check_option_value(const OptionDesc *desc, OptionValue val)
{
    switch (desc->type) {
    case OPT_STR:
        BUG_ON(!val.str_val);
        BUG_ON(val.str_val != str_intern(val.str_val));
        if (desc->u.str_opt.validate) {
            BUG_ON(!desc->u.str_opt.validate(val.str_val));
        }
        return;
    case OPT_UINT:
        BUG_ON(val.uint_val < desc->u.uint_opt.min);
        BUG_ON(val.uint_val > desc->u.uint_opt.max);
        return;
    case OPT_ENUM:
        BUG_ON(val.uint_val >= count_enum_values(desc));
        return;
    case OPT_FLAG: {
            size_t nvals = count_enum_values(desc);
            BUG_ON(nvals >= 32);
            unsigned int mask = (1u << nvals) - 1;
            unsigned int uint_val = val.uint_val;
            BUG_ON((uint_val & mask) != uint_val);
        }
        return;
    case OPT_REGEX:
        BUG_ON(val.str_val && val.str_val[0] == '\0');
        BUG_ON(val.str_val && !regexp_is_interned(val.str_val));
        return;
    case OPT_BOOL:
        return;
    }

    BUG("unhandled option type");
}

static void sanity_check_options(const void *opts, bool global)
{
    for (size_t i = 0; i < ARRAYLEN(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        BUG_ON(desc->type >= ARRAYLEN(option_ops));
        if ((desc->global && desc->local) || global == desc->global) {
            OptionValue val = desc_get(desc, (char*)opts + desc->offset);
            sanity_check_option_value(desc, val);
        }
    }
}

void sanity_check_global_options(const GlobalOptions *gopts)
{
    sanity_check_options(gopts, true);
}

void sanity_check_local_options(const LocalOptions *lopts)
{
    sanity_check_options(lopts, false);
}
#endif

void collect_options(PointerArray *a, const char *prefix, bool local, bool global)
{
    for (size_t i = 0; i < ARRAYLEN(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        if ((local && !desc->local) || (global && !desc->global)) {
            continue;
        }
        if (str_has_prefix(desc->name, prefix)) {
            ptr_array_append(a, xstrdup(desc->name));
        }
    }
}

// Collect options that can be set via the "option" command
void collect_auto_options(PointerArray *a, const char *prefix)
{
    for (size_t i = 0; i < ARRAYLEN(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        if (!desc->local || desc->on_change == filetype_changed) {
            continue;
        }
        if (str_has_prefix(desc->name, prefix)) {
            ptr_array_append(a, xstrdup(desc->name));
        }
    }
}

void collect_toggleable_options(PointerArray *a, const char *prefix, bool global)
{
    for (size_t i = 0; i < ARRAYLEN(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        if (global && !desc->global) {
            continue;
        }
        OptionType type = desc->type;
        bool toggleable = (type == OPT_ENUM || type == OPT_BOOL);
        if (toggleable && str_has_prefix(desc->name, prefix)) {
            ptr_array_append(a, xstrdup(desc->name));
        }
    }
}

void collect_option_values(EditorState *e, PointerArray *a, const char *option, const char *prefix)
{
    const OptionDesc *desc = find_option(option);
    if (!desc) {
        return;
    }

    if (prefix[0] == '\0') {
        char *ptr = get_option_ptr(e, desc, !desc->local);
        OptionValue value = desc_get(desc, ptr);
        ptr_array_append(a, xstrdup(desc_string(desc, value)));
        return;
    }

    OptionType type = desc->type;
    if (type == OPT_STR || type == OPT_UINT || type == OPT_REGEX) {
        return;
    }

    const char *const *values = desc->u.enum_opt.values;
    if (type == OPT_ENUM || type == OPT_BOOL) {
        for (size_t i = 0; values[i]; i++) {
            if (str_has_prefix(values[i], prefix)) {
                ptr_array_append(a, xstrdup(values[i]));
            }
        }
        return;
    }

    BUG_ON(type != OPT_FLAG);
    const char *comma = strrchr(prefix, ',');
    size_t prefix_len = comma ? ++comma - prefix : 0;
    for (size_t i = 0; values[i]; i++) {
        const char *str = values[i];
        if (str_has_prefix(str, prefix + prefix_len)) {
            size_t str_len = strlen(str);
            char *completion = xmalloc(prefix_len + str_len + 1);
            memcpy(completion, prefix, prefix_len);
            memcpy(completion + prefix_len, str, str_len + 1);
            ptr_array_append(a, completion);
        }
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

String dump_options(GlobalOptions *gopts, LocalOptions *lopts)
{
    String buf = string_new(4096);
    for (size_t i = 0; i < ARRAYLEN(option_desc); i++) {
        const OptionDesc *desc = &option_desc[i];
        void *local = desc->local ? local_ptr(desc, lopts) : NULL;
        void *global = desc->global ? global_ptr(desc, gopts) : NULL;
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
    return buf;
}

const char *get_option_value_string(EditorState *e, const char *name)
{
    const OptionDesc *desc = find_option(name);
    if (!desc) {
        return NULL;
    }
    char *ptr = get_option_ptr(e, desc, !desc->local);
    return desc_string(desc, desc_get(desc, ptr));
}
