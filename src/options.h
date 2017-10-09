#ifndef OPTIONS_H
#define OPTIONS_H

#include "libc.h"

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

typedef struct {
    bool auto_indent;
    int detect_indent;
    bool emulate_tab;
    bool expand_tab;
    bool file_history;
    int indent_width;
    bool syntax;
    int tab_width;
    int text_width;
    int ws_error;
} CommonOptions;

// TODO: Embed CommonOptions as a C11 "unnamed struct" instead of duplicating
typedef struct {
    // These have also global values
    bool auto_indent;
    int detect_indent;
    bool emulate_tab;
    bool expand_tab;
    bool file_history;
    int indent_width;
    bool syntax;
    int tab_width;
    int text_width;
    int ws_error;

    // Only local
    bool brace_indent;
    char *filetype;
    char *indent_regex;
} LocalOptions;

typedef struct {
    // These have also local values
    bool auto_indent;
    int detect_indent;
    bool emulate_tab;
    bool expand_tab;
    bool file_history;
    int indent_width;
    bool syntax;
    int tab_width;
    int text_width;
    int ws_error;

    // Only global
    SearchCaseSensitivity case_sensitive_search;
    bool display_special;
    int esc_timeout;
    bool lock_files;
    LineEndingType newline; // Default value for new files
    int scroll_margin;
    bool show_line_numbers;
    char *statusline_left;
    char *statusline_right;
    enum tab_bar tab_bar;
    int tab_bar_max_components;
    int tab_bar_width;
} GlobalOptions;

extern const char *case_sensitive_search_enum[];

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
