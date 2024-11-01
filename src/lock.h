#ifndef LOCK_H
#define LOCK_H

#include <stdbool.h>
#include <sys/types.h>
#include "util/macros.h"

typedef struct {
    char *locks;
    char *locks_lock;
    mode_t locks_mode;
    pid_t editor_pid;
} FileLocksContext;

void init_file_locks_context(FileLocksContext *ctx, const char *fallback_dir, pid_t pid) NONNULL_ARGS;
void free_file_locks_context(FileLocksContext *ctx) NONNULL_ARGS;
bool lock_file(const FileLocksContext *ctx, const char *filename) NONNULL_ARGS WARN_UNUSED_RESULT;
void unlock_file(const FileLocksContext *ctx, const char *filename) NONNULL_ARGS;

#endif
