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

extern ErrorBuffer errbuf; // NOLINT(*-avoid-non-const-global-variables)

// These wrapper macros are much like the functions below, but using the
// global errbuf (temporarily, while the code transitions away from it)
#define error_msg_(...) error_msg(&errbuf, __VA_ARGS__)
#define error_msg_errno_(...) error_msg_errno(&errbuf, __VA_ARGS__)

struct EditorState;

bool error_msg(ErrorBuffer *eb, const char *format, ...) COLD PRINTF(2) NONNULL_ARGS;
bool error_msg_errno(ErrorBuffer *eb, const char *prefix) COLD NONNULL_ARGS;
bool error_msg_for_cmd(struct EditorState *e, const char *cmd, const char *format, ...) COLD PRINTF(3) NONNULL_ARG(1, 3);
bool info_msg(ErrorBuffer *eb, const char *format, ...) PRINTF(2) NONNULL_ARGS;
void clear_error(ErrorBuffer *eb) NONNULL_ARGS;

#endif
