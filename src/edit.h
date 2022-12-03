#ifndef EDIT_H
#define EDIT_H

#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"
#include "view.h"

void do_insert(View *view, const char *buf, size_t len) NONNULL_ARG(1);
char *do_delete(View *view, size_t len, bool sanity_check_newlines) NONNULL_ARGS;
char *do_replace(View *view, size_t del, const char *buf, size_t ins) NONNULL_ARGS_AND_RETURN;

#endif
