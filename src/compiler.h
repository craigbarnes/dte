#ifndef COMPILER_H
#define COMPILER_H

#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

typedef struct {
    bool ignore;
    int8_t msg_idx;
    int8_t file_idx;
    int8_t line_idx;
    int8_t column_idx;
    const char *pattern; // Original pattern string (interned)
    regex_t re; // Compiled pattern
} ErrorFormat;

typedef struct {
    PointerArray error_formats;
} Compiler;

void add_error_fmt(const char *compiler, bool ignore, const char *format, char **desc) NONNULL_ARGS;
Compiler *find_compiler(const char *name) NONNULL_ARGS;
void collect_compilers(const char *prefix) NONNULL_ARGS;
String dump_compiler(const Compiler *c, const char *name) NONNULL_ARGS;
String dump_compilers(void);

#endif
