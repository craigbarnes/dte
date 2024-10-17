#ifndef INSERT_H
#define INSERT_H

#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"
#include "util/unicode.h"
#include "view.h"

void insert_text(View *view, const char *text, size_t size, bool move_after) NONNULL_ARG(1);
void insert_ch(View *view, CodePoint ch) NONNULL_ARGS;
void new_line(View *view, bool above) NONNULL_ARGS;

#endif
