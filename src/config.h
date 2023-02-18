#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include "command/run.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/string.h"

typedef enum {
    CFG_NOFLAGS = 0,
    CFG_MUST_EXIST = 1 << 0,
    CFG_BUILTIN = 1 << 1
} ConfigFlags;

typedef struct {
    const char *const name;
    const StringView text;
} BuiltinConfig;

typedef struct {
    const char *file;
    unsigned int line;
} ConfigState;

extern ConfigState current_config;

struct EditorState;

String dump_builtin_configs(void);
const BuiltinConfig *get_builtin_config(const char *name) PURE;
const BuiltinConfig *get_builtin_configs_array(size_t *nconfigs);
void exec_config(CommandRunner *runner, StringView config);
int do_read_config(CommandRunner *runner, const char *filename, ConfigFlags flags) WARN_UNUSED_RESULT;
int read_config(CommandRunner *runner, const char *filename, ConfigFlags f);
void exec_builtin_color_reset(struct EditorState *e);
void exec_builtin_rc(struct EditorState *e);
void collect_builtin_configs(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_builtin_includes(PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
