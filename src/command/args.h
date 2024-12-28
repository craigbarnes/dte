#ifndef COMMAND_ARGS_H
#define COMMAND_ARGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "run.h"
#include "util/base64.h"
#include "util/debug.h"
#include "util/macros.h"

typedef enum {
    ARGERR_NONE,
    ARGERR_INVALID_OPTION,
    ARGERR_TOO_MANY_OPTIONS,
    ARGERR_OPTION_ARGUMENT_MISSING,
    ARGERR_OPTION_ARGUMENT_NOT_SEPARATE,
    ARGERR_TOO_FEW_ARGUMENTS,
    ARGERR_TOO_MANY_ARGUMENTS,
} ArgParseError;

typedef struct {
    char ch;
    unsigned int val;
} FlagMapping;

static inline CommandArgs cmdargs_new(char **args)
{
    return (CommandArgs){.args = args};
}

// Map ASCII alphanumeric characters to values between 1 and 62,
// for use as bitset indices in CommandArgs::flag_set
static inline unsigned int cmdargs_flagset_idx(unsigned char c)
{
    // We use base64_decode() here simply because it does a similar byte
    // mapping to the one we want and allows us to share the lookup table.
    // There's no "real" base64 encoding involved.
    static_assert(BASE64_INVALID > 63);
    unsigned int idx = IS_CT_CONSTANT(c) ? base64_decode_branchy(c) : base64_decode(c);
    BUG_ON(idx > 61);
    return idx + 1;
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

static inline char cmdargs_pick_winning_flag_from_set(const CommandArgs *a, uint_least64_t flagset)
{
    if (!(a->flag_set & flagset)) {
        return 0;
    }

    BUG_ON(a->nr_flags > ARRAYLEN(a->flags));
    for (size_t n = a->nr_flags, i = n - 1; i < n; i--) {
        char flag = a->flags[i];
        if (flagset & cmdargs_flagset_value(flag)) {
            return flag;
        }
    }

    return 0;
}

static inline char cmdargs_pick_winning_flag(const CommandArgs *a, char f1, char f2)
{
    uint_least64_t set = cmdargs_flagset_value(f1) | cmdargs_flagset_value(f2);
    char flag = cmdargs_pick_winning_flag_from_set(a, set);
    BUG_ON(flag && flag != f1 && flag != f2);
    return flag;
}

// Convert CommandArgs::flag_set to bit flags of a different format,
// by using a set of mappings
static inline unsigned int cmdargs_convert_flags (
    const CommandArgs *a,
    const FlagMapping *map,
    size_t map_len
) {
    unsigned int val = 0;
    UNROLL_LOOP(16)
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
