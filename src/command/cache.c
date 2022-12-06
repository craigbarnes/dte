#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "args.h"
#include "parse.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

CachedCommand *cached_command_new(const CommandRunner *runner, const char *cmd_str)
{
    const size_t cmd_str_len = strlen(cmd_str);
    CachedCommand *cached = xmalloc(sizeof(*cached) + cmd_str_len + 1);
    memcpy(cached->cmd_str, cmd_str, cmd_str_len + 1);

    PointerArray array = PTR_ARRAY_INIT;
    if (parse_commands(runner, &array, cmd_str) != CMDERR_NONE) {
        goto nocache;
    }

    ptr_array_trim_nulls(&array);
    size_t n = array.count;
    if (n < 2 || ptr_array_idx(&array, NULL) != n - 1) {
        // Only single commands can be cached
        goto nocache;
    }

    const Command *cmd = runner->cmds->lookup(array.ptrs[0]);
    if (!cmd) {
        // Aliases or non-existent commands can't be cached
        goto nocache;
    }

    if (memchr(cmd_str, '$', cmd_str_len)) {
        // Commands containing variables can't be cached
        goto nocache;
    }

    free(ptr_array_remove_idx(&array, 0));
    CommandArgs cmdargs = cmdargs_new((char**)array.ptrs);
    if (do_parse_args(cmd, &cmdargs) != ARGERR_NONE) {
        goto nocache;
    }

    // Command can be cached; cache takes ownership of args array
    cached->cmd = cmd;
    cached->a = cmdargs;
    return cached;

nocache:
    ptr_array_free(&array);
    cached->cmd = NULL;
    return cached;
}

void cached_command_free(CachedCommand *cc)
{
    if (!cc) {
        return;
    }
    if (cc->cmd) {
        free_string_array(cc->a.args);
    }
    free(cc);
}
