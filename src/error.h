#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include "util/macros.h"

bool error_msg(const char *format, ...) COLD PRINTF(1);
bool error_msg_errno(const char *prefix) COLD NONNULL_ARGS;
bool error_msg_for_cmd(const char *cmd, const char *format, ...) COLD PRINTF(2);
bool info_msg(const char *format, ...) PRINTF(1);
void clear_error(void);
const char *get_msg(bool *is_error) NONNULL_ARGS;
unsigned int get_nr_errors(void);
void errors_to_stderr(bool enable);

#endif
