#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include "command/run.h"
#include "util/align.h"
#include "util/errorcode.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/string.h"

// Used in the generated headers included by src/config.c and test/config.c
#define CFG(n, t) {.name = n, .text = {.data = t, .length = sizeof(t) - 1}}

#ifdef __linux__
    #define CONFIG_SECTION SECTION(".dte.config") MAXALIGN
#else
    #define CONFIG_SECTION
#endif

typedef enum {
    CFG_NOFLAGS = 0,
    CFG_MUST_EXIST = 1 << 0, // Show an error when attempting to read non-existent configs
    CFG_BUILTIN = 1 << 1, // Look up filenames in builtin_configs[], instead of in the filesystem
} ConfigFlags;

typedef struct {
    const char *const name;
    const StringView text;
} BuiltinConfig;

struct EditorState;

String dump_builtin_configs(void) WARN_UNUSED_RESULT;
const BuiltinConfig *get_builtin_config(const char *name) PURE NONNULL_ARGS WARN_UNUSED_RESULT;
const BuiltinConfig *get_builtin_configs_array(size_t *nconfigs) NONNULL_ARGS_AND_RETURN WRITEONLY(1) WARN_UNUSED_RESULT;
bool exec_config(CommandRunner *runner, StringView config) NONNULL_ARGS WARN_UNUSED_RESULT;
SystemErrno read_config(CommandRunner *runner, const char *filename, ConfigFlags f) NONNULL_ARGS WARN_UNUSED_RESULT;
void exec_builtin_color_reset(struct EditorState *e) NONNULL_ARGS;
void exec_rc_files(struct EditorState *e, const char *filename, bool read_user_rc) NONNULL_ARG(1);
void collect_builtin_configs(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_builtin_config_variables(PointerArray *a, StringView prefix) NONNULL_ARGS;
void collect_builtin_includes(PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
