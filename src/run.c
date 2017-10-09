#include "command.h"
#include "error.h"
#include "alias.h"
#include "parse-args.h"
#include "change.h"
#include "config.h"
#include "common.h"

// Commands that are allowed in config files
static const char *const config_commands[] = {
    "alias",
    "bind",
    "cd",
    "errorfmt",
    "ft",
    "hi",
    "include",
    "load-syntax",
    "option",
    "set",
    "setenv",
};

const Command *current_command;

static bool allowed_command(const char *name)
{
    for (size_t i = 0; i < ARRAY_COUNT(config_commands); i++) {
        if (streq(name, config_commands[i])) {
            return true;
        }
    }
    return false;
}

const Command *find_command(const Command *cmds, const char *name)
{
    for (size_t i = 0; cmds[i].name; i++) {
        const Command *cmd = &cmds[i];
        if (streq(name, cmd->name)) {
            return cmd;
        }
    }
    return NULL;
}

static void run_command(const Command *cmds, char **av)
{
    const Command *cmd = find_command(cmds, av[0]);
    const char *pf;
    char **args;

    if (!cmd) {
        PointerArray array = PTR_ARRAY_INIT;
        const char *alias_name = av[0];
        const char *alias_value = find_alias(alias_name);
        Error *err = NULL;

        if (alias_value == NULL) {
            error_msg("No such command or alias: %s", alias_name);
            return;
        }
        if (!parse_commands(&array, alias_value, &err)) {
            error_msg("Parsing alias %s: %s", alias_name, err->msg);
            error_free(err);
            ptr_array_free(&array);
            return;
        }

        // Remove NULL
        array.count--;

        for (size_t i = 1; av[i]; i++) {
            ptr_array_add(&array, xstrdup(av[i]));
        }
        ptr_array_add(&array, NULL);

        run_commands(cmds, &array);
        ptr_array_free(&array);
        return;
    }

    if (config_file && cmds == commands && !allowed_command(cmd->name)) {
        error_msg("Command %s not allowed in config file.", cmd->name);
        return;
    }

    // By default change can't be merged with previous on.
    // Any command can override this by calling begin_change() again.
    begin_change(CHANGE_MERGE_NONE);

    current_command = cmd;
    args = av + 1;
    pf = parse_args(args, cmd->flags, cmd->min_args, cmd->max_args);
    if (pf) {
        cmd->cmd(pf, args);
    }
    current_command = NULL;

    end_change();
}

void run_commands(const Command *cmds, const PointerArray *array)
{
    size_t s, e;

    s = 0;
    while (s < array->count) {
        e = s;
        while (e < array->count && array->ptrs[e]) {
            e++;
        }

        if (e > s) {
            run_command(cmds, (char **)array->ptrs + s);
        }

        s = e + 1;
    }
}

void handle_command(const Command *cmds, const char *cmd)
{
    Error *err = NULL;
    PointerArray array = PTR_ARRAY_INIT;

    if (!parse_commands(&array, cmd, &err)) {
        error_msg("%s", err->msg);
        error_free(err);
        ptr_array_free(&array);
        return;
    }

    run_commands(cmds, &array);
    ptr_array_free(&array);
}
