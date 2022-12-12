#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include "util/macros.h"

bool error_msg(const char *format, ...) COLD PRINTF(1);
bool error_msg_errno(const char *prefix) COLD NONNULL_ARGS;
void info_msg(const char *format, ...) PRINTF(1);
void clear_error(void);
const char *get_msg(bool *is_error) NONNULL_ARGS;
unsigned int get_nr_errors(void);
void set_print_errors_to_stderr(bool enable);

#endif
