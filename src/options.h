#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include "regexp.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

enum {
    INDENT_WIDTH_MAX = 8,
    TAB_WIDTH_MAX = 8,
    TEXT_WIDTH_MAX = 1000,
};

// Note: this must be kept in sync with ws_error_values[]
typedef enum {
    WSE_SPACE_INDENT = 1 << 0, // Spaces in indent (except WSE_SPACE_ALIGN)
    WSE_SPACE_ALIGN = 1 << 1, // Less than tab-width spaces at end of indent
    WSE_TAB_INDENT = 1 << 2, // Tab in indent
    WSE_TAB_AFTER_INDENT = 1 << 3, // Tab anywhere but indent
    WSE_SPECIAL = 1 << 4, // Special whitespace characters
    WSE_AUTO_INDENT = 1 << 5, // expand-tab ? WSE_TAB_AFTER_INDENT | WSE_TAB_INDENT : WSE_SPACE_INDENT
    WSE_TRAILING = 1 << 6, // Trailing whitespace
    WSE_ALL_TRAILING = 1 << 7, // Like WSE_TRAILING, but including around cursor
} WhitespaceErrorFlags;

// Note: this must be kept in sync with save_unmodified_enum[]
typedef enum {
    SAVE_NONE,
    SAVE_TOUCH,
    SAVE_FULL,
} SaveUnmodifiedType;

#define COMMON_OPTIONS \
    unsigned int detect_indent; \
    unsigned int indent_width; \
    unsigned int save_unmodified; \
    unsigned int tab_width; \
    unsigned int text_width; \
    unsigned int ws_error; \
    bool auto_indent; \
    bool editorconfig; \
    bool emulate_tab; \
    bool expand_tab; \
    bool file_history; \
    bool fsync; \
    bool overwrite; \
    bool syntax

typedef struct {
    COMMON_OPTIONS;
} CommonOptions;

// Note: all members should be initialized in buffer_new()
typedef struct {
    COMMON_OPTIONS;
    // Only local
    bool brace_indent;
    const char *filetype;
    const InternedRegexp *indent_regex;
} LocalOptions;

typedef struct {
    COMMON_OPTIONS;
    // Only global
    bool display_special;
    bool lock_files;
    bool optimize_true_color;
    bool select_cursor_char;
    bool set_window_title;
    bool show_line_numbers;
    bool tab_bar;
    bool utf8_bom; // Default value for new files
    unsigned int esc_timeout;
    unsigned int filesize_limit;
    unsigned int scroll_margin;
    unsigned int crlf_newlines; // Default value for new files
    unsigned int case_sensitive_search; // SearchCaseSensitivity
    const char *statusline_left;
    const char *statusline_right;
} GlobalOptions;

#undef COMMON_OPTIONS

static inline bool use_spaces_for_indent(const LocalOptions *opt)
{
    return opt->expand_tab || opt->indent_width != opt->tab_width;
}

struct EditorState;

bool set_option(struct EditorState *e, const char *name, const char *value, bool local, bool global);
bool set_bool_option(struct EditorState *e, const char *name, bool local, bool global);
bool toggle_option(struct EditorState *e, const char *name, bool global, bool verbose);
bool toggle_option_values(struct EditorState *e, const char *name, bool global, bool verbose, char **values, size_t count);
bool validate_local_options(char **strs);
void collect_options(PointerArray *a, const char *prefix, bool local, bool global);
void collect_auto_options(PointerArray *a, const char *prefix);
void collect_toggleable_options(PointerArray *a, const char *prefix, bool global);
void collect_option_values(struct EditorState *e, PointerArray *a, const char *option, const char *prefix);
String dump_options(GlobalOptions *gopts, LocalOptions *lopts);
const char *get_option_value_string(struct EditorState *e, const char *name);

#if DEBUG >= 1
    void sanity_check_global_options(const GlobalOptions *opts);
    void sanity_check_local_options(const LocalOptions *lopts);
#else
    static inline void sanity_check_global_options(const GlobalOptions* UNUSED_ARG(gopts)) {}
    static inline void sanity_check_local_options(const LocalOptions* UNUSED_ARG(lopts)) {}
#endif

#endif
