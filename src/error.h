#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include "util/macros.h"

typedef struct {
    char *msg;
    int code;
} Error;

extern const char *const error_ptr;
extern unsigned int nr_errors;
extern bool msg_is_error;

Error *error_create(const char *format, ...) PRINTF(1);
Error *error_create_errno(int code, const char *format, ...) PRINTF(2);
void error_free(Error *err);

void clear_error(void);
void error_msg(const char *format, ...) PRINTF(1);
void info_msg(const char *format, ...) PRINTF(1);

#endif
