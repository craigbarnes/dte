#ifndef WRAP_H
#define WRAP_H

#include <stddef.h>
#include "util/macros.h"
#include "view.h"

void wrap_paragraph(View *view, size_t text_width) NONNULL_ARGS;

#endif
