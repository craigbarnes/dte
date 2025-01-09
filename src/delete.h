#ifndef DELETE_H
#define DELETE_H

#include <stdbool.h>
#include "util/macros.h"
#include "view.h"

void delete_ch(View *view) NONNULL_ARGS;
void erase(View *view) NONNULL_ARGS;
void clear_lines(View *view, bool auto_indent) NONNULL_ARGS;

#endif
