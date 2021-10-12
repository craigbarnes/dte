#ifndef LOCK_H
#define LOCK_H

#include <stdbool.h>
#include "util/macros.h"

bool lock_file(const char *filename) WARN_UNUSED_RESULT;
void unlock_file(const char *filename);

#endif
