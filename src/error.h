#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include "util/macros.h"

void error_msg(const char *format, ...) PRINTF(1);
void info_msg(const char *format, ...) PRINTF(1);
void clear_error(void);
const char *get_msg(bool *is_error) NONNULL_ARGS;
unsigned int get_nr_errors(void);
void suppress_error_msg(void);
void unsuppress_error_msg(void);

#endif
