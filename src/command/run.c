#include "run.h"
#include "alias.h"
#include "args.h"
#include "parse.h"
#include "change.h"
#include "config.h"
#include "error.h"
#include "macro.h"
#include "util/ptr-array.h"
#include "util/xmalloc.h"

const Command *current_command;

static bool run_commands(CommandRunner *runner, const PointerArray *array);

static bool run_command(CommandRunner *runner, char **av)
{
    const CommandSet *cmds = runner->cmds;
    const Command *cmd = cmds->lookup(av[0]);
    if (!cmd) {
        const char *name = av[0];
        if (!runner->lookup_alias) {
            return error_msg("No such command: %s", name);
        }

        const char *alias_value = runner->lookup_alias(name, runner->userdata);
        if (unlikely(!alias_value)) {
            return error_msg("No such command or alias: %s", name);
        }

        PointerArray array = PTR_ARRAY_INIT;
        CommandParseError err = parse_commands(runner, &array, alias_value);
        if (unlikely(err != CMDERR_NONE)) {
            const char *err_msg = command_parse_error_to_string(err);
            ptr_array_free(&array);
            return error_msg("Parsing alias %s: %s", name, err_msg);
        }

        // Remove NULL
        array.count--;

        for (size_t i = 1; av[i]; i++) {
            ptr_array_append(&array, xstrdup(av[i]));
        }
        ptr_array_append(&array, NULL);

        bool r = run_commands(runner, &array);
        ptr_array_free(&array);
        return r;
    }

    if (unlikely(current_config.file && !cmd->allow_in_rc)) {
        return error_msg("Command %s not allowed in config file", cmd->name);
    }

    // Record command in macro buffer, if recording (this needs to be done
    // before parse_args() mutates the array)
    if (runner->allow_recording && runner->cmds->macro_record) {
        runner->cmds->macro_record(cmd, av + 1, runner->userdata);
    }

    // By default change can't be merged with previous one.
    // Any command can override this by calling begin_change() again.
    begin_change(CHANGE_MERGE_NONE);

    CommandArgs a = cmdargs_new(av + 1);
    current_command = cmd;
    bool r = likely(parse_args(cmd, &a)) && cmd->cmd(runner->userdata, &a);
    current_command = NULL;

    end_change();
    return r;
}

static bool run_commands(CommandRunner *runner, const PointerArray *array)
{
    if (unlikely(runner->recursion_count > 16)) {
        return error_msg("alias recursion overflow");
    }

    void **ptrs = array->ptrs;
    size_t len = array->count;
    size_t nfailed = 0;
    BUG_ON(len == 0);
    BUG_ON(ptrs[len - 1] != NULL);
    runner->recursion_count++;

    for (size_t s = 0, e = 0; s < len; ) {
        // Iterate over strings, until a terminating NULL is encountered
        while (ptrs[e]) {
            e++;
            BUG_ON(e >= len);
        }

        // If the value of `e` (end) changed, there's a run of at least
        // 1 string, which is a command followed by 0 or more arguments
        if (e != s) {
            if (!run_command(runner, (char**)ptrs + s)) {
                nfailed++;
            }
        }

        // Skip past the NULL, onto the next command (if any)
        s = ++e;
    }

    runner->recursion_count--;
    return (nfailed == 0);
}

bool handle_command(CommandRunner *runner, const char *cmd)
{
    BUG_ON(runner->recursion_count != 0);
    PointerArray array = PTR_ARRAY_INIT;
    CommandParseError err = parse_commands(runner, &array, cmd);
    bool r;
    if (likely(err == CMDERR_NONE)) {
        r = run_commands(runner, &array);
        BUG_ON(runner->recursion_count != 0);
    } else {
        const char *str = command_parse_error_to_string(err);
        error_msg("Command syntax error: %s", str);
        r = false;
    }
    ptr_array_free(&array);
    return r;
}
