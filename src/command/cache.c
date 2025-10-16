#include <stdlib.h>
#include <string.h>
#include "cache.h"
#include "args.h"
#include "parse.h"
#include "trace.h"
#include "util/ptr-array.h"
#include "util/str-array.h"
#include "util/xmalloc.h"

// Takes a command string and returns a struct containing the resolved
// Command and pre-parsed arguments, or a NULL Command if uncacheable.
// In both cases, CachedCommand::cmd_str is filled with a copy of the
// original string. This caching is done to allow handle_binding() to
// avoid repeated parsing/allocation each time a key binding is used.
CachedCommand *cached_command_new(const CommandRunner *runner, const char *cmd_str)
{
    const size_t cmd_str_len = strlen(cmd_str);
    CachedCommand *cached = xmalloc(xadd3(sizeof(*cached), cmd_str_len, 1));
    memcpy(cached->cmd_str, cmd_str, cmd_str_len + 1);

    const char *reason;
    PointerArray array = PTR_ARRAY_INIT;
    if (parse_commands(runner, &array, cmd_str) != CMDERR_NONE) {
        reason = "parsing failed";
        goto nocache;
    }

    ptr_array_trim_nulls(&array);
    size_t n = array.count;
    if (n < 2 || ptr_array_xindex(&array, NULL) != n - 1) {
        reason = "multiple commands";
        goto nocache;
    }

    const Command *cmd = runner->cmds->lookup(array.ptrs[0]);
    if (!cmd) {
        // Aliases and non-existent commands can't be cached, because the
        // command they expand to could later be invalidated by cmd_alias().
        reason = "contains aliases";
        goto nocache;
    }

    // TODO: Make this condition more precise
    if (memchr(cmd_str, '$', cmd_str_len)) {
        reason = "may contain variables";
        goto nocache;
    }

    free(ptr_array_remove_index(&array, 0));
    CommandArgs cmdargs = cmdargs_new((char**)array.ptrs);
    if (do_parse_args(cmd, &cmdargs) != ARGERR_NONE) {
        reason = "argument parsing failed";
        goto nocache;
    }

    // Command can be cached; cache takes ownership of args array
    cached->cmd = cmd;
    cached->a = cmdargs;
    return cached;

nocache:
    TRACE_CMD("skipping command cache (%s): %s", reason, cmd_str);
    ptr_array_free(&array);
    cached->cmd = NULL;
    return cached;
}

void cached_command_free(CachedCommand *cc)
{
    if (cc && cc->cmd) {
        free_string_array(cc->a.args);
    }
    free(cc);
}
