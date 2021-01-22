#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include "command/run.h"
#include "util/macros.h"
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
    int line;
} ConfigState;

extern ConfigState current_config;

String dump_builtin_configs(void);
void collect_builtin_configs(const char *prefix);
const BuiltinConfig *get_builtin_config(const char *name) PURE;
const BuiltinConfig *get_builtin_configs_array(size_t *nconfigs);
void exec_config(const CommandSet *cmds, const char *buf, size_t size);
int do_read_config(const CommandSet *cmds, const char *filename, ConfigFlags f);
int read_config(const CommandSet *cmds, const char *filename, ConfigFlags f);
void exec_builtin_color_reset(void);
void exec_builtin_rc(void);

#endif
