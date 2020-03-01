#include "alias.h"
#include "change.h"
#include "command.h"
#include "debug.h"
#include "config.h"
#include "error.h"
#include "parse-args.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

const Command *current_command;

static bool allowed_command(const char *name)
{
    size_t len = strlen(name);
    switch (len) {
    case 3: return mem_equal(name, "set", len);
    case 4: return mem_equal(name, "bind", len);
    case 5: return mem_equal(name, "alias", len);
    case 7: return mem_equal(name, "include", len);
    case 8: return mem_equal(name, "errorfmt", len);
    case 11: return mem_equal(name, "load-syntax", len);
    case 2:
        switch (name[0]) {
        case 'c': return name[1] == 'd';
        case 'f': return name[1] == 't';
        case 'h': return name[1] == 'i';
        }
        return false;
    case 6:
        switch (name[0]) {
        case 'o': return mem_equal(name, "option", len);
        case 's': return mem_equal(name, "setenv", len);
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

static void run_commands(const CommandSet *cmds, const PointerArray *array);

static void run_command(const CommandSet *cmds, char **av)
{
    const Command *cmd = cmds->lookup(av[0]);
    if (!cmd) {
        const char *alias_name = av[0];
        const char *alias_value = find_alias(alias_name);
        if (alias_value == NULL) {
            error_msg("No such command or alias: %s", alias_name);
            return;
        }

        PointerArray array = PTR_ARRAY_INIT;
        CommandParseError err = parse_commands(&array, alias_value);
        if (err != CMDERR_NONE) {
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

        run_commands(cmds, &array);
        ptr_array_free(&array);
        return;
    }

    if (config_file && cmds == &commands && !allowed_command(cmd->name)) {
        error_msg("Command %s not allowed in config file.", cmd->name);
        return;
    }

    // By default change can't be merged with previous one.
    // Any command can override this by calling begin_change() again.
    begin_change(CHANGE_MERGE_NONE);

    CommandArgs a = {.args = av + 1};
    current_command = cmd;
    if (parse_args(cmd, &a)) {
        cmd->cmd(&a);
    }
    current_command = NULL;

    end_change();
}

static void run_commands(const CommandSet *cmds, const PointerArray *array)
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
            run_command(cmds, (char **)array->ptrs + s);
        }

        s = e + 1;
    }

out:
    BUG_ON(recursion_count == 0);
    recursion_count--;
}

void handle_command(const CommandSet *cmds, const char *cmd)
{
    PointerArray array = PTR_ARRAY_INIT;
    CommandParseError err = parse_commands(&array, cmd);
    if (err == CMDERR_NONE) {
        run_commands(cmds, &array);
    } else {
        const char *str = command_parse_error_to_string(err);
        error_msg("Command syntax error: %s", str);
    }
    ptr_array_free(&array);
}
