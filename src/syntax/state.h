#ifndef SYNTAX_STATE_H
#define SYNTAX_STATE_H

#include "config.h"
#include "editor.h"
#include "syntax/syntax.h"

Syntax *do_load_syntax_file(HashMap *syntaxes, const char *filename, ConfigFlags flags, int *err);
Syntax *load_syntax_file(EditorState *e, const char *filename, ConfigFlags flags, int *err);
Syntax *load_syntax_by_filetype(EditorState *e, const char *filetype);

#endif
