#include <stdlib.h>
#include "parse-args.h"
#include "common.h"
#include "error.h"

/*
 * Flags and first "--" are removed.
 * Flag arguments are moved to beginning.
 * Other arguments come right after flag arguments.
 *
 * Returns parsed flags (order is preserved).
 */
const char *parse_args(char **args, const char *flag_desc, int min, int max)
{
    static char flags[16];
    size_t argc = count_strings(args);
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
                return NULL;
            }
            flags[nr_flags++] = flag;
            if (nr_flags == ARRAY_COUNT(flags)) {
                error_msg("Too many options given.");
                return NULL;
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
                return NULL;
            }
            flag_arg = args[i + 1];
            if (!flag_arg) {
                error_msg("Option -%c requires on argument.", flag);
                return NULL;
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

    if ((int)argc < min) {
        error_msg("Not enough arguments");
        return NULL;
    }
    if (max >= 0 && (int)argc > max) {
        error_msg("Too many arguments");
        return NULL;
    }
    flags[nr_flags] = '\0';
    return flags;
}
