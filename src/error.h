#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include "util/macros.h"

typedef struct {
    int code;
    char msg[];
} Error;

extern const char *const error_ptr;
extern unsigned int nr_errors;
extern bool msg_is_error;
extern bool supress_error_msg;

#define error_create(...) error_create_errno(0, __VA_ARGS__)

Error *error_create_errno(int code, const char *format, ...) PRINTF(2);
void error_free(Error *err);

void clear_error(void);
void error_msg(const char *format, ...) PRINTF(1);
void info_msg(const char *format, ...) PRINTF(1);

#endif
