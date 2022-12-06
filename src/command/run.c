#include "run.h"
#include "alias.h"
#include "args.h"
#include "parse.h"
#include "change.h"
#include "config.h"
#include "error.h"
#include "macro.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/xmalloc.h"

const Command *current_command;

static void run_commands(CommandRunner *runner, const PointerArray *array);

static void run_command(CommandRunner *runner, char **av)
{
    const CommandSet *cmds = runner->cmds;
    const Command *cmd = cmds->lookup(av[0]);
    if (!cmd) {
        const char *name = av[0];
        if (!runner->aliases) {
            error_msg("No such command: %s", name);
            return;
        }

        const char *alias_value = find_alias(runner->aliases, name);
        if (unlikely(!alias_value)) {
            error_msg("No such command or alias: %s", name);
            return;
        }

        PointerArray array = PTR_ARRAY_INIT;
        CommandParseError err = parse_commands(runner, &array, alias_value);
        if (unlikely(err != CMDERR_NONE)) {
            const char *err_msg = command_parse_error_to_string(err);
            error_msg("Parsing alias %s: %s", name, err_msg);
            ptr_array_free(&array);
            return;
        }

        // Remove NULL
        array.count--;

        for (size_t i = 1; av[i]; i++) {
            ptr_array_append(&array, xstrdup(av[i]));
        }
        ptr_array_append(&array, NULL);

        run_commands(runner, &array);
        ptr_array_free(&array);
        return;
    }

    if (unlikely(current_config.file && !cmd->allow_in_rc)) {
        error_msg("Command %s not allowed in config file", cmd->name);
        return;
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
    if (likely(parse_args(cmd, &a))) {
        cmd->cmd(runner->userdata, &a);
    }
    current_command = NULL;

    end_change();
}

static void run_commands(CommandRunner *runner, const PointerArray *array)
{
    if (unlikely(runner->recursion_count++ > 16)) {
        error_msg("alias recursion overflow");
        goto out;
    }

    void **ptrs = array->ptrs;
    size_t len = array->count;
    BUG_ON(len == 0);
    BUG_ON(ptrs[len - 1] != NULL);

    for (size_t s = 0, e = 0; s < len; ) {
        // Iterate over strings, until a terminating NULL is encountered
        while (ptrs[e]) {
            e++;
            BUG_ON(e >= len);
        }

        // If the value of `e` (end) changed, there's a run of at least
        // 1 string, which is a command followed by 0 or more arguments
        if (e != s) {
            run_command(runner, (char**)ptrs + s);
        }

        // Skip past the NULL, onto the next command (if any)
        s = ++e;
    }

out:
    runner->recursion_count--;
}

void handle_command(CommandRunner *runner, const char *cmd)
{
    BUG_ON(runner->recursion_count != 0);
    PointerArray array = PTR_ARRAY_INIT;
    CommandParseError err = parse_commands(runner, &array, cmd);
    if (likely(err == CMDERR_NONE)) {
        run_commands(runner, &array);
        BUG_ON(runner->recursion_count != 0);
    } else {
        const char *str = command_parse_error_to_string(err);
        error_msg("Command syntax error: %s", str);
    }
    ptr_array_free(&array);
}
