#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include "util/macros.h"

extern const char *const error_ptr;
extern unsigned int nr_errors;
extern bool msg_is_error;
extern bool supress_error_msg;

void error_msg(const char *format, ...) PRINTF(1);
void info_msg(const char *format, ...) PRINTF(1);
void clear_error(void);

#endif
