#ifndef STATE_H
#define STATE_H

#include <stdbool.h>
#include "config.h"
#include "syntax.h"

Syntax *load_syntax_file(const char *filename, ConfigFlags f, int *err);
Syntax *load_syntax_by_filetype(const char *filetype);

#endif
