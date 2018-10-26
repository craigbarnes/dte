#ifndef SYNTAX_STATE_H
#define SYNTAX_STATE_H

#include <stdbool.h>
#include "syntax.h"
#include "../config.h"

Syntax *load_syntax_file(const char *filename, ConfigFlags f, int *err);
Syntax *load_syntax_by_filetype(const char *filetype);

#endif
