#ifndef STATE_H
#define STATE_H

#include <stdbool.h>
#include "config.h"

struct syntax *load_syntax_file(const char *filename, ConfigFlags f, int *err);
struct syntax *load_syntax_by_filetype(const char *filetype);

#endif
