#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>

typedef enum {
    NEWLINE_UNIX,
    NEWLINE_DOS,
} LineEndingType;

enum {
    // Trailing whitespace
    WSE_TRAILING = 1 << 0,

    // Spaces in indentation.
    // Does not include less than tab-width spaces at end of indentation.
    WSE_SPACE_INDENT = 1 << 1,

    // Less than tab-width spaces at end of indentation
    WSE_SPACE_ALIGN = 1 << 2,

    // Tab in indentation
    WSE_TAB_INDENT = 1 << 3,

    // Tab anywhere but in indentation
    WSE_TAB_AFTER_INDENT = 1 << 4,

    // Special whitespace characters
    WSE_SPECIAL = 1 << 5,

    // expand-tab = false: WSE_SPACE_INDENT
    // expand-tab = true:  WSE_TAB_AFTER_INDENT | WSE_TAB_INDENT
    WSE_AUTO_INDENT = 1 << 6,
};

typedef enum {
    CSS_FALSE,
    CSS_TRUE,
    CSS_AUTO,
} SearchCaseSensitivity;

enum tab_bar {
    TAB_BAR_HIDDEN,
    TAB_BAR_HORIZONTAL,
    TAB_BAR_VERTICAL,
    TAB_BAR_AUTO,
};

#define COMMON_OPTIONS \
    int auto_indent; \
    int detect_indent; \
    int emulate_tab; \
    int expand_tab; \
    int file_history; \
    int indent_width; \
    int syntax; \
    int tab_width; \
    int text_width; \
    int ws_error

typedef struct {
    COMMON_OPTIONS;
} CommonOptions;

typedef struct {
    COMMON_OPTIONS;
    // Only local
    int brace_indent;
    char *filetype;
    char *indent_regex;
} LocalOptions;

typedef struct {
    COMMON_OPTIONS;
    // Only global
    SearchCaseSensitivity case_sensitive_search;
    int display_special;
    int esc_timeout;
    int lock_files;
    LineEndingType newline; // Default value for new files
    int scroll_margin;
    int set_window_title;
    int show_line_numbers;
    const char *statusline_left;
    const char *statusline_right;
    enum tab_bar tab_bar;
    int tab_bar_max_components;
    int tab_bar_width;
} GlobalOptions;

#undef COMMON_OPTIONS

#define TAB_BAR_MIN_WIDTH 12

void set_option(const char *name, const char *value, bool local, bool global);
void set_bool_option(const char *name, bool local, bool global);
void toggle_option(const char *name, bool global, bool verbose);
void toggle_option_values(const char *name, bool global, bool verbose, char **values);
bool validate_local_options(char **strs);
void collect_options(const char *prefix);
void collect_toggleable_options(const char *prefix);
void collect_option_values(const char *name, const char *prefix);
void free_local_options(LocalOptions *opt);
const char *case_sensitivity_to_string(SearchCaseSensitivity s);

#endif
