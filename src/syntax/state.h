#ifndef SYNTAX_STATE_H
#define SYNTAX_STATE_H

#include "config.h"
#include "syntax/syntax.h"

Syntax *load_syntax_file(const char *filename, ConfigFlags f, int *err);
Syntax *load_syntax_by_filetype(const char *filetype);

#endif
