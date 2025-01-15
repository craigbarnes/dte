#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include "util/macros.h"

typedef struct ErrorBuffer {
    char buf[512];
    unsigned int nr_errors;
    bool is_error;
    bool print_to_stderr;
} ErrorBuffer;

struct EditorState;

bool error_msg(ErrorBuffer *eb, const char *format, ...) COLD PRINTF(2) NONNULL_ARGS;
bool error_msg_errno(ErrorBuffer *eb, const char *prefix) COLD NONNULL_ARGS;
bool error_msg_for_cmd(struct EditorState *e, const char *cmd, const char *format, ...) COLD PRINTF(3) NONNULL_ARG(1, 3);
bool info_msg(ErrorBuffer *eb, const char *format, ...) PRINTF(2) NONNULL_ARGS;
void clear_error(ErrorBuffer *eb) NONNULL_ARGS;

#endif
