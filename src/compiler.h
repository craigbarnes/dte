#ifndef COMPILER_H
#define COMPILER_H

#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "util/hashmap.h"
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
    NR_ERRFMT_INDICES
};

typedef struct {
    int8_t capture_index[NR_ERRFMT_INDICES];
    bool ignore;
    const char *pattern; // Original pattern string (interned)
    regex_t re; // Compiled pattern
} ErrorFormat;

typedef struct {
    PointerArray error_formats;
} Compiler;

static inline Compiler *find_compiler(const HashMap *compilers, const char *name)
{
    return hashmap_get(compilers, name);
}

void remove_compiler(HashMap *compilers, const char *name) NONNULL_ARGS;
void free_compiler(Compiler *c) NONNULL_ARGS;
void free_error_format(ErrorFormat *f) NONNULL_ARGS;
ssize_t errorfmt_capture_name_to_index(const char *name) NONNULL_ARGS WARN_UNUSED_RESULT;
void collect_errorfmt_capture_names(PointerArray *a, const char *prefix) NONNULL_ARGS;
void dump_compiler(const Compiler *c, const char *name, String *s) NONNULL_ARGS;

NONNULL_ARGS
void add_error_fmt (
    HashMap *compilers,
    const char *name,
    const char *pattern,
    regex_t *re, // Takes ownership
    int8_t idx[static NR_ERRFMT_INDICES],
    bool ignore
);

#endif
