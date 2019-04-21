#ifndef EDITORCONFIG_H
#define EDITORCONFIG_H

#include <stdbool.h>

typedef enum {
    INDENT_STYLE_UNSPECIFIED,
    INDENT_STYLE_TAB,
    INDENT_STYLE_SPACE,
} IndentStyle;

typedef struct {
    unsigned int indent_size;
    unsigned int tab_width;
    unsigned int max_line_length;
    IndentStyle indent_style;
    bool indent_size_is_tab;
} EditorConfigOptions;

int editorconfig_parse(const char *full_filename, EditorConfigOptions *opts);

#endif
