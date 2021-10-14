#ifndef COMPILER_H
#define COMPILER_H

#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

enum {
    ERRORFMT_CAPTURE_MAX = 16
};

enum {
    ERRFMT_FILE,
    ERRFMT_LINE,
    ERRFMT_COLUMN,
    ERRFMT_MESSAGE,
};

typedef struct {
    int8_t capture_index[4];
    bool ignore;
    const char *pattern; // Original pattern string (interned)
    regex_t re; // Compiled pattern
} ErrorFormat;

typedef struct {
    PointerArray error_formats;
} Compiler;

void add_error_fmt(const char *compiler, bool ignore, const char *format, char **desc) NONNULL_ARGS;
Compiler *find_compiler(const char *name) NONNULL_ARGS;
void collect_compilers(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_errorfmt_capture_names(PointerArray *a, const char *prefix) NONNULL_ARGS;
String dump_compiler(const Compiler *c, const char *name) NONNULL_ARGS;
String dump_compilers(void);

#endif
