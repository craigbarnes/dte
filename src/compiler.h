#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>
#include <stdint.h>
#include "ptr-array.h"
#include "regexp.h"

typedef struct {
    char *file;
    char *msg;
    int line;
    int column;
} CompileError;

typedef struct {
    bool ignore;
    int8_t msg_idx;
    int8_t file_idx;
    int8_t line_idx;
    int8_t column_idx;
    regex_t re;
} ErrorFormat;

typedef struct {
    char *name;
    PointerArray error_formats;
} Compiler;

void add_error_fmt(const char *compiler, bool ignore, const char *format, char **desc);
Compiler *find_compiler(const char *name);

#endif
