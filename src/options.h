#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>
#include <stddef.h>
#include "util/string.h"

enum {
    // Spaces in indentation.
    // Does not include less than tab-width spaces at end of indentation.
    WSE_SPACE_INDENT = 1 << 0,

    // Less than tab-width spaces at end of indentation
    WSE_SPACE_ALIGN = 1 << 1,

    // Tab in indentation
    WSE_TAB_INDENT = 1 << 2,

    // Tab anywhere but in indentation
    WSE_TAB_AFTER_INDENT = 1 << 3,

    // Special whitespace characters
    WSE_SPECIAL = 1 << 4,

    // expand-tab = false: WSE_SPACE_INDENT
    // expand-tab = true:  WSE_TAB_AFTER_INDENT | WSE_TAB_INDENT
    WSE_AUTO_INDENT = 1 << 5,

    // Trailing whitespace
    WSE_TRAILING = 1 << 6,

    // Like WSE_TRAILING, but also includes runs of whitespace adjacent
    // to the cursor (where the presence of trailing whitespace is usually
    // obvious and highlighting can be annoying)
    WSE_ALL_TRAILING = 1 << 7,
};

typedef enum {
    CSS_FALSE,
    CSS_TRUE,
    CSS_AUTO,
} SearchCaseSensitivity;

#define COMMON_OPTIONS \
    unsigned int detect_indent; \
    unsigned int indent_width; \
    unsigned int tab_width; \
    unsigned int text_width; \
    unsigned int ws_error; \
    bool auto_indent; \
    bool editorconfig; \
    bool emulate_tab; \
    bool expand_tab; \
    bool file_history; \
    bool fsync; \
    bool syntax

typedef struct {
    COMMON_OPTIONS;
} CommonOptions;

typedef struct {
    COMMON_OPTIONS;
    // Only local
    bool brace_indent;
    const char *filetype;
    const char *indent_regex;
} LocalOptions;

typedef struct {
    COMMON_OPTIONS;
    // Only global
    bool display_invisible;
    bool display_special;
    bool lock_files;
    bool select_cursor_char;
    bool set_window_title;
    bool show_line_numbers;
    bool tab_bar;
    bool utf8_bom; // Default value for new files
    unsigned int esc_timeout;
    unsigned int filesize_limit;
    unsigned int scroll_margin;
    unsigned int crlf_newlines; // Default value for new files
    unsigned int case_sensitive_search;
    const char *statusline_left;
    const char *statusline_right;
} GlobalOptions;

#undef COMMON_OPTIONS

void set_option(const char *name, const char *value, bool local, bool global);
void set_bool_option(const char *name, bool local, bool global);
void toggle_option(const char *name, bool global, bool verbose);
void toggle_option_values(const char *name, bool global, bool verbose, char **values, size_t count);
bool validate_local_options(char **strs);
void collect_options(const char *prefix, bool local, bool global);
void collect_auto_options(const char *prefix);
void collect_toggleable_options(const char *prefix, bool global);
void collect_option_values(const char *option, const char *prefix);
String dump_options(void);
const char *get_option_value_string(const char *name);

#endif
