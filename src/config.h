#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include "command/run.h"
#include "util/align.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/string.h"

#ifdef __linux__
    // Used in generated headers (`build/gen/{test-data,builtin-config}*.h`)
    // included by `src/config.c` and `test/config.c`
    #define CONFIG_SECTION SECTION(".dte.config") MAXALIGN
#else
    #define CONFIG_SECTION
#endif

typedef enum {
    CFG_NOFLAGS = 0,
    CFG_MUST_EXIST = 1 << 0,
    CFG_BUILTIN = 1 << 1,
} ConfigFlags;

typedef struct {
    const char *const name;
    const StringView text;
} BuiltinConfig;

struct EditorState;

String dump_builtin_configs(void);
const BuiltinConfig *get_builtin_config(const char *name) PURE;
const BuiltinConfig *get_builtin_configs_array(size_t *nconfigs);
bool exec_config(CommandRunner *runner, StringView config);
int read_config(CommandRunner *runner, const char *filename, ConfigFlags f);
void exec_builtin_color_reset(struct EditorState *e);
void exec_rc_files(struct EditorState *e, const char *filename, bool read_user_rc) NONNULL_ARG(1);
void collect_builtin_configs(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_builtin_includes(PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
