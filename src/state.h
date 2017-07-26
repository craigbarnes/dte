#ifndef STATE_H
#define STATE_H

#include "libc.h"

struct syntax *load_syntax_file(const char *filename, bool must_exist, int *err);
struct syntax *load_syntax_by_filetype(const char *filetype);

#endif
