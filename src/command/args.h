#ifndef COMMAND_ARGS_H
#define COMMAND_ARGS_H

#include <stdbool.h>
#include <stdint.h>
#include "run.h"
#include "util/base64.h"
#include "util/debug.h"
#include "util/macros.h"

typedef enum {
    ARGERR_NONE,
    ARGERR_INVALID_OPTION,
    ARGERR_TOO_MANY_OPTIONS,
    ARGERR_OPTION_ARGUMENT_NOT_SEPARATE,
    ARGERR_OPTION_ARGUMENT_MISSING,
    ARGERR_TOO_FEW_ARGUMENTS,
    ARGERR_TOO_MANY_ARGUMENTS,
} ArgParseError;

typedef struct {
    char ch;
    unsigned int val;
} FlagMapping;

static inline CommandArgs cmdargs_new(char **args, void *userdata)
{
    return (CommandArgs) {
        .args = args,
        .userdata = userdata
    };
}

// Map ASCII alphanumeric characters to values between 1 and 62,
// for use as bitset indices in CommandArgs::flag_set
static inline unsigned int cmdargs_flagset_idx(unsigned char flag)
{
    // We use base64_decode() here simply because it does a similar byte
    // mapping to the one we want and allows us to share the lookup table.
    // There's no "real" base64 encoding involved.
    static_assert(BASE64_INVALID > 63);
    unsigned int idx = base64_decode(flag) + 1;
    BUG_ON(idx > 62);
    return idx;
}

static inline uint_least64_t cmdargs_flagset_value(unsigned char flag)
{
    return UINT64_C(1) << cmdargs_flagset_idx(flag);
}

static inline bool cmdargs_has_flag(const CommandArgs *a, unsigned char flag)
{
    uint_least64_t bitmask = cmdargs_flagset_value(flag);
    static_assert_compatible_types(bitmask, a->flag_set);
    return (a->flag_set & bitmask) != 0;
}

// Convert CommandArgs::flag_set to bit flags of a different format,
// by using a set of mappings
static inline unsigned int cmdargs_convert_flags (
    const CommandArgs *a,
    const FlagMapping *map,
    size_t map_len
) {
    unsigned int val = 0;
    for (size_t i = 0; i < map_len; i++, map++) {
        if (cmdargs_has_flag(a, map->ch)) {
            val |= map->val;
        }
    }
    return val;
}

bool parse_args(const Command *cmd, CommandArgs *a) NONNULL_ARGS WARN_UNUSED_RESULT;
ArgParseError do_parse_args(const Command *cmd, CommandArgs *a) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
