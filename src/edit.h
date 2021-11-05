#ifndef EDIT_H
#define EDIT_H

#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"

void do_insert(const char *buf, size_t len);
char *do_delete(size_t len, bool sanity_check_newlines);
char *do_replace(size_t del, const char *buf, size_t ins);

#endif
