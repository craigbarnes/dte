#ifndef SYNTAX_MERGE_H
#define SYNTAX_MERGE_H

#include <stddef.h>
#include "syntax/color.h"
#include "syntax/syntax.h"
#include "util/macros.h"

typedef struct {
    Syntax *subsyn;
    State *return_state;
    const char *delim;
    size_t delim_len;
} SyntaxMerge;

State *merge_syntax(Syntax *syn, SyntaxMerge *m, const ColorScheme *colors) NONNULL_ARGS_AND_RETURN;

#endif
