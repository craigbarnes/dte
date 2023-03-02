#ifndef EDITORCONFIG_EDITORCONFIG_H
#define EDITORCONFIG_EDITORCONFIG_H

#include <stdbool.h>
#include "util/macros.h"

typedef enum {
    INDENT_STYLE_UNSPECIFIED,
    INDENT_STYLE_TAB,
    INDENT_STYLE_SPACE,
} EditorConfigIndentStyle;

typedef struct {
    unsigned int indent_size;
    unsigned int tab_width;
    unsigned int max_line_length;
    EditorConfigIndentStyle indent_style;
    bool indent_size_is_tab;
} EditorConfigOptions;

static inline EditorConfigOptions editorconfig_options_init(void)
{
    return (EditorConfigOptions){.indent_style = INDENT_STYLE_UNSPECIFIED};
}

NONNULL_ARG(1) WARN_UNUSED_RESULT
int get_editorconfig_options(const char *pathname, EditorConfigOptions *opts);

#endif
