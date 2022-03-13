#ifndef LOCK_H
#define LOCK_H

#include <stdbool.h>
#include <sys/types.h>
#include "util/macros.h"

void init_file_locks_context(const char *fallback_dir, pid_t pid);
bool lock_file(const char *filename) WARN_UNUSED_RESULT;
void unlock_file(const char *filename);

#endif
