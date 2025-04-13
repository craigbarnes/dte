#ifndef COMMAND_ARGS_H
#define COMMAND_ARGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "error.h"
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

static inline CommandFlagSet cmdargs_flagset_bit(unsigned char flag)
{
    return UINT64_C(1) << cmdargs_flagset_idx(flag);
}

static inline CommandFlagSet cmdargs_flagset_from_str(const char *flags)
{
    CommandFlagSet set = 0;
    UNROLL_LOOP(16)
    for (size_t i = 0, n = strlen(flags); i < n; i++) {
        set |= cmdargs_flagset_bit(flags[i]);
    }
    return set;
}

static inline bool cmdargs_has_flag(const CommandArgs *a, unsigned char flag)
{
    CommandFlagSet bitmask = cmdargs_flagset_bit(flag);
    static_assert_compatible_types(bitmask, a->flag_set);
    return (a->flag_set & bitmask) != 0;
}

// Return the rightmost flag in CommandArgs::flags[] that is also a member of
// `flagset`, or 0 if no such flags are present. CommandFunc handlers can use
// this to accept multiple, mutually exclusive flags and ignore all except the
// last. For example, `exec -s -t -s` is equivalent to `exec -s`.
static inline char cmdargs_pick_winning_flag_from_set(const CommandArgs *a, CommandFlagSet flagset)
{
    if (!(a->flag_set & flagset)) {
        return 0;
    }

    BUG_ON(a->nr_flags > ARRAYLEN(a->flags));
    for (size_t n = a->nr_flags, i = n - 1; i < n; i--) {
        char flag = a->flags[i];
        if (flagset & cmdargs_flagset_bit(flag)) {
            return flag;
        }
    }

    return 0;
}

static inline char cmdargs_pick_winning_flag(const CommandArgs *a, const char *flagstr)
{
    CommandFlagSet set = cmdargs_flagset_from_str(flagstr);
    return cmdargs_pick_winning_flag_from_set(a, set);
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

bool parse_args(const Command *cmd, CommandArgs *a, ErrorBuffer *ebuf) NONNULL_ARGS WARN_UNUSED_RESULT;
ArgParseError do_parse_args(const Command *cmd, CommandArgs *a) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
