#ifndef ERROR_H
#define ERROR_H

#include "libc.h"

struct error {
	char *msg;
	int code;
};

extern int nr_errors;
extern bool msg_is_error;
extern char error_buf[256];

struct error *error_create(const char *format, ...) FORMAT(1);
struct error *error_create_errno(int code, const char *format, ...);
void error_free(struct error *err);

void clear_error(void);
void error_msg(const char *format, ...) FORMAT(1);
void info_msg(const char *format, ...) FORMAT(1);

#endif
