#ifndef SHOW_H
#define SHOW_H

#include <stdbool.h>
#include "util/macros.h"

void show(const char *type, const char *key, bool cflag) NONNULL_ARG(1);

#endif
