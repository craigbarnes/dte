#ifndef COMMAND_ARGS_H
#define COMMAND_ARGS_H

#include <stdbool.h>
#include <stdint.h>
#include "run.h"
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

static inline uint8_t cmdargs_flagset_idx(unsigned char flag)
{
    extern const uint8_t flagset_map[128];
    if (unlikely(flag >= sizeof(flagset_map))) {
        return 0;
    }
    uint8_t idx = flagset_map[flag];
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
