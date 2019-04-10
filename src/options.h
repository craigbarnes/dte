#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>
#include "encoding/encoder.h"

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

typedef enum {
    TAB_BAR_HIDDEN,
    TAB_BAR_HORIZONTAL,
    TAB_BAR_VERTICAL,
    TAB_BAR_AUTO,
} TabBarMode;

#define COMMON_OPTIONS \
    unsigned int auto_indent; \
    unsigned int detect_indent; \
    unsigned int editorconfig; \
    unsigned int emulate_tab; \
    unsigned int expand_tab; \
    unsigned int file_history; \
    unsigned int indent_width; \
    unsigned int syntax; \
    unsigned int tab_width; \
    unsigned int text_width; \
    unsigned int ws_error

typedef struct {
    COMMON_OPTIONS;
} CommonOptions;

typedef struct {
    COMMON_OPTIONS;
    // Only local
    unsigned int brace_indent;
    char *filetype;
    char *indent_regex;
} LocalOptions;

typedef struct {
    COMMON_OPTIONS;
    // Only global
    unsigned int display_special;
    unsigned int esc_timeout;
    unsigned int filesize_limit;
    unsigned int lock_files;
    unsigned int scroll_margin;
    unsigned int set_window_title;
    unsigned int show_line_numbers;
    unsigned int tab_bar_max_components;
    unsigned int tab_bar_width;
    LineEndingType newline; // Default value for new files
    SearchCaseSensitivity case_sensitive_search;
    TabBarMode tab_bar;
    const char *statusline_left;
    const char *statusline_right;
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

#endif
