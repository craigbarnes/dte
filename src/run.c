#include "alias.h"
#include "change.h"
#include "command.h"
#include "common.h"
#include "debug.h"
#include "config.h"
#include "error.h"
#include "parse-args.h"
#include "util/xmalloc.h"

const Command *current_command;

static PURE bool allowed_command(const char *name)
{
    size_t len = strlen(name);
    switch (len) {
    case 3: return !memcmp(name, "set", len);
    case 4: return !memcmp(name, "bind", len);
    case 5: return !memcmp(name, "alias", len);
    case 7: return !memcmp(name, "include", len);
    case 8: return !memcmp(name, "errorfmt", len);
    case 11: return !memcmp(name, "load-syntax", len);
    case 2:
        switch (name[0]) {
        case 'c': return name[1] == 'd';
        case 'f': return name[1] == 't';
        case 'h': return name[1] == 'i';
        }
        return false;
    case 6:
        switch (name[0]) {
        case 'o': return !memcmp(name, "option", len);
        case 's': return !memcmp(name, "setenv", len);
        }
        return false;
    }
    return false;
}

UNITTEST {
    BUG_ON(!allowed_command("alias"));
    BUG_ON(!allowed_command("cd"));
    BUG_ON(!allowed_command("include"));
    BUG_ON(!allowed_command("set"));
    BUG_ON(allowed_command("alias_"));
    BUG_ON(allowed_command("c"));
    BUG_ON(allowed_command("cD"));
}

const Command *find_command(const Command *cmds, const char *name)
{
    for (size_t i = 0; cmds[i].cmd; i++) {
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

    // By default change can't be merged with previous one.
    // Any command can override this by calling begin_change() again.
    begin_change(CHANGE_MERGE_NONE);

    current_command = cmd;
    char **args = av + 1;
    const char *pf = parse_args(args, cmd->flags, cmd->min_args, cmd->max_args);
    if (pf) {
        cmd->cmd(pf, args);
    }
    current_command = NULL;

    end_change();
}

void run_commands(const Command *cmds, const PointerArray *array)
{
    size_t s = 0;
    while (s < array->count) {
        size_t e = s;
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
