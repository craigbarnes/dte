#ifndef COMPILER_H
#define COMPILER_H

#include "ptr-array.h"
#include "regexp.h"
#include "libc.h"

typedef struct {
    char *file;
    char *msg;
    int line;
    int column;
} CompileError;

typedef struct {
    bool ignore;
    signed char msg_idx;
    signed char file_idx;
    signed char line_idx;
    signed char column_idx;
    regex_t re;
} ErrorFormat;

typedef struct {
    char *name;
    PointerArray error_formats;
} Compiler;

void add_error_fmt(const char *compiler, bool ignore, const char *format, char **desc);
Compiler *find_compiler(const char *name);

#endif
