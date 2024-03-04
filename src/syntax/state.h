#ifndef SYNTAX_STATE_H
#define SYNTAX_STATE_H

#include "editor.h"
#include "syntax/syntax.h"

typedef enum {
    SYN_MUST_EXIST = 1 << 0, // See CFG_MUST_EXIST and syn_flags_to_cfg_flags()
    SYN_BUILTIN = 1 << 1, // See CFG_BUILTIN
    SYN_WARN_ON_UNUSED_SUBSYN = 1 << 2, // See cmd_require() and cmd_syntax()
    SYN_LINT = 1 << 3, // Perform extra linting (see: lint_emit_name())
} SyntaxLoadFlags;

Syntax *load_syntax_file(EditorState *e, const char *filename, SyntaxLoadFlags flags, int *err);
Syntax *load_syntax_by_filetype(EditorState *e, const char *filetype);

#endif
