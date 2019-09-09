#include <stdlib.h>
#include "parse-args.h"
#include "debug.h"
#include "error.h"
#include "util/str-util.h"

/*
 * Flags and first "--" are removed.
 * Flag arguments are moved to beginning.
 * Other arguments come right after flag arguments.
 *
 * a->args field should be set before calling.
 * If parsing succeeds, the other field are set and true is returned.
 */
bool parse_args(const Command *cmd, CommandArgs *a)
{
    char **args = a->args;
    BUG_ON(!args);

    size_t argc = 0;
    while (args[argc]) {
        argc++;
    }

    const char *flag_desc = cmd->flags;
    size_t nr_flags = 0;
    size_t nr_flag_args = 0;
    bool flags_after_arg = true;

    if (*flag_desc == '-') {
        flag_desc++;
        flags_after_arg = false;
    }

    size_t i = 0;
    while (args[i]) {
        char *arg = args[i];

        if (streq(arg, "--")) {
            // Move the NULL too
            memmove(args + i, args + i + 1, (argc - i) * sizeof(*args));
            free(arg);
            argc--;
            break;
        }
        if (arg[0] != '-' || !arg[1]) {
            if (!flags_after_arg) {
                break;
            }
            i++;
            continue;
        }

        for (size_t j = 1; arg[j]; j++) {
            char flag = arg[j];
            char *flag_arg;
            char *flagp = strchr(flag_desc, flag);

            if (!flagp || flag == '=') {
                error_msg("Invalid option -%c", flag);
                return false;
            }
            a->flags[nr_flags++] = flag;
            if (nr_flags == ARRAY_COUNT(a->flags)) {
                error_msg("Too many options given.");
                return false;
            }
            if (flagp[1] != '=') {
                continue;
            }

            if (j > 1 || arg[j + 1]) {
                error_msg (
                    "Flag -%c must be given separately because it"
                    " requires an argument.",
                    flag
                );
                return false;
            }
            flag_arg = args[i + 1];
            if (!flag_arg) {
                error_msg("Option -%c requires on argument.", flag);
                return false;
            }

            // Move flag argument before any other arguments
            if (i != nr_flag_args) {
                // farg1 arg1  arg2 -f   farg2 arg3
                // farg1 farg2 arg1 arg2 arg3
                size_t count = i - nr_flag_args;
                memmove (
                    args + nr_flag_args + 1,
                    args + nr_flag_args,
                    count * sizeof(*args)
                );
            }
            args[nr_flag_args++] = flag_arg;
            i++;
        }

        memmove(args + i, args + i + 1, (argc - i) * sizeof(*args));
        free(arg);
        argc--;
    }

    // Don't count arguments to flags as arguments to command
    argc -= nr_flag_args;

    if (argc < cmd->min_args) {
        error_msg("Not enough arguments");
        return false;
    }
    if (argc > cmd->max_args) {
        error_msg("Too many arguments");
        return false;
    }
    a->flags[nr_flags] = '\0';

    a->nr_args = argc;
    a->nr_flags = nr_flags;
    return true;
}
