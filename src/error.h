#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include "util/macros.h"

void error_msg(const char *format, ...) PRINTF(1);
void perror_msg(const char *prefix) NONNULL_ARGS;
void info_msg(const char *format, ...) PRINTF(1);
void clear_error(void);
const char *get_msg(bool *is_error) NONNULL_ARGS;
unsigned int get_nr_errors(void);

#endif
