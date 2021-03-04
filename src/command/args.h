#ifndef COMMAND_ARGS_H
#define COMMAND_ARGS_H

#include <stdbool.h>
#include "run.h"
#include "util/base64.h"
#include "util/debug.h"
#include "util/macros.h"

typedef enum {
    ARGERR_INVALID_OPTION = 1,
    ARGERR_TOO_MANY_OPTIONS,
    ARGERR_OPTION_ARGUMENT_NOT_SEPARATE,
    ARGERR_OPTION_ARGUMENT_MISSING,
    ARGERR_TOO_FEW_ARGUMENTS,
    ARGERR_TOO_MANY_ARGUMENTS,
} ArgParseErrorType;

// Success: 0
// Failure: (ARGERR_* | (flag << 8))
typedef unsigned int ArgParseError;

// Map ASCII alphanumeric characters to values between 1 and 62,
// for use as bitset indices in CommandArgs::flag_set
static inline unsigned int cmdargs_flagset_idx(unsigned char flag)
{
    // We use base64_decode() here simply because it does a similar byte
    // mapping to the one we want and allows us to share the lookup table.
    // There's no "real" base64 encoding involved.
    unsigned int idx = base64_decode(flag) + 1;
    BUG_ON(idx > 62);
    return idx;
}

static inline bool cmdargs_has_flag(const CommandArgs *a, unsigned char flag)
{
    uint64_t bitmask = UINT64_C(1) << cmdargs_flagset_idx(flag);
    static_assert_compatible_types(bitmask, a->flag_set);
    return (a->flag_set & bitmask) != 0;
}

bool parse_args(const Command *cmd, CommandArgs *a) NONNULL_ARGS;
ArgParseError do_parse_args(const Command *cmd, CommandArgs *a) NONNULL_ARGS;

#endif
