#ifndef JOIN_H
#define JOIN_H

#include <stddef.h>
#include "view.h"
#include "util/macros.h"

void join_lines(View *view, const char *delim, size_t delim_len) NONNULL_ARG(1);

#endif
