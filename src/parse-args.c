#include <stdlib.h>
#include "parse-args.h"
#include "debug.h"
#include "error.h"
#include "util/str-util.h"

static void set_error(ArgParseError *err, ArgParseErrorType type, char flag)
{
    if (err) {
        *err = (ArgParseError){.type = type, .flag = flag};
    }
}

/*
 * Flags and first "--" are removed.
 * Flag arguments are moved to beginning.
 * Other arguments come right after flag arguments.
 *
 * a->args field should be set before calling.
 * If parsing succeeds, the other field are set and true is returned.
 */
bool do_parse_args(const Command *cmd, CommandArgs *a, ArgParseError *err)
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
                set_error(err, ARGERR_INVALID_OPTION, flag);
                return false;
            }
            a->flags[nr_flags++] = flag;
            if (nr_flags == ARRAY_COUNT(a->flags)) {
                set_error(err, ARGERR_TOO_MANY_OPTIONS, '\0');
                return false;
            }
            if (flagp[1] != '=') {
                continue;
            }

            if (j > 1 || arg[j + 1]) {
                set_error(err, ARGERR_OPTION_ARGUMENT_NOT_SEPARATE, flag);
                return false;
            }
            flag_arg = args[i + 1];
            if (!flag_arg) {
                set_error(err, ARGERR_OPTION_ARGUMENT_MISSING, flag);
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
        set_error(err, ARGERR_TOO_FEW_ARGUMENTS, '\0');
        return false;
    }
    if (argc > cmd->max_args) {
        set_error(err, ARGERR_TOO_MANY_ARGUMENTS, '\0');
        return false;
    }
    a->flags[nr_flags] = '\0';

    a->nr_args = argc;
    a->nr_flags = nr_flags;
    return true;
}

bool parse_args(const Command *cmd, CommandArgs *a)
{
    ArgParseError err;
    if (do_parse_args(cmd, a, &err)) {
        return true;
    }
    switch (err.type) {
    case ARGERR_INVALID_OPTION:
        error_msg("Invalid option -%c", err.flag);
        break;
    case ARGERR_TOO_MANY_OPTIONS:
        error_msg("Too many options given");
        break;
    case ARGERR_OPTION_ARGUMENT_NOT_SEPARATE:
        error_msg (
            "Flag -%c must be given separately because it"
            " requires an argument",
            err.flag
        );
        break;
    case ARGERR_OPTION_ARGUMENT_MISSING:
        error_msg("Option -%c requires an argument", err.flag);
        break;
    case ARGERR_TOO_FEW_ARGUMENTS:
        error_msg("Not enough arguments");
        break;
    case ARGERR_TOO_MANY_ARGUMENTS:
        error_msg("Too many arguments");
        break;
    default:
        BUG("unhandled error type");
    }
    return false;
}
