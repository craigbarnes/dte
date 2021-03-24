#include "run.h"
#include "args.h"
#include "parse.h"
#include "alias.h"
#include "change.h"
#include "config.h"
#include "error.h"
#include "macro.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/xmalloc.h"

const Command *current_command;

static void run_commands(const CommandSet *cmds, const PointerArray *array, bool allow_recording);

static void run_command(const CommandSet *cmds, char **av, bool allow_recording)
{
    const Command *cmd = cmds->lookup(av[0]);
    if (!cmd) {
        const char *alias_name = av[0];
        const char *alias_value = find_alias(alias_name);
        if (unlikely(!alias_value)) {
            error_msg("No such command or alias: %s", alias_name);
            return;
        }

        PointerArray array = PTR_ARRAY_INIT;
        CommandParseError err = parse_commands(&array, alias_value);
        if (unlikely(err != CMDERR_NONE)) {
            const char *err_msg = command_parse_error_to_string(err);
            error_msg("Parsing alias %s: %s", alias_name, err_msg);
            ptr_array_free(&array);
            return;
        }

        // Remove NULL
        array.count--;

        for (size_t i = 1; av[i]; i++) {
            ptr_array_append(&array, xstrdup(av[i]));
        }
        ptr_array_append(&array, NULL);

        run_commands(cmds, &array, allow_recording);
        ptr_array_free(&array);
        return;
    }

    if (unlikely(current_config.file && !cmd->allow_in_rc)) {
        error_msg("Command %s not allowed in config file.", cmd->name);
        return;
    }

    // Record command in macro buffer, if recording (this needs to be done
    // before parse_args() mutates the array)
    if (allow_recording && cmds->allow_recording(cmd, av + 1)) {
        macro_command_hook(cmd->name, av + 1);
    }

    // By default change can't be merged with previous one.
    // Any command can override this by calling begin_change() again.
    begin_change(CHANGE_MERGE_NONE);

    CommandArgs a = {.args = av + 1};
    current_command = cmd;
    if (likely(parse_args(cmd, &a))) {
        cmd->cmd(&a);
    }
    current_command = NULL;

    end_change();
}

static void run_commands(const CommandSet *cmds, const PointerArray *array, bool allow_recording)
{
    static unsigned int recursion_count;
    if (unlikely(recursion_count++ > 16)) {
        error_msg("alias recursion overflow");
        goto out;
    }

    size_t s = 0;
    while (s < array->count) {
        size_t e = s;
        while (e < array->count && array->ptrs[e]) {
            e++;
        }

        if (e > s) {
            run_command(cmds, (char **)array->ptrs + s, allow_recording);
        }

        s = e + 1;
    }

out:
    recursion_count--;
}

void handle_command(const CommandSet *cmds, const char *cmd, bool allow_recording)
{
    PointerArray array = PTR_ARRAY_INIT;
    CommandParseError err = parse_commands(&array, cmd);
    if (likely(err == CMDERR_NONE)) {
        run_commands(cmds, &array, allow_recording);
    } else {
        const char *str = command_parse_error_to_string(err);
        error_msg("Command syntax error: %s", str);
    }
    ptr_array_free(&array);
}
