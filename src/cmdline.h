#ifndef CMDLINE_H
#define CMDLINE_H

#include <stdbool.h>
#include <sys/types.h>
#include "command/run.h"
#include "history.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/string.h"

typedef struct {
    char *orig; // Full cmdline string (backing buffer for `escaped` and `tail`)
    char *parsed; // Result of passing `escaped` through parse_command_arg()
    StringView escaped; // Middle part of `orig` (string to be replaced)
    StringView tail; // Suffix part of `orig` (after `escaped`)
    size_t head_len; // Length of prefix part of `orig` (before `escaped`)
    PointerArray completions; // Array of completion candidates
    size_t idx; // Index of currently selected completion
    bool add_space_after_single_match;
    bool tilde_expanded;
} CompletionState;

typedef struct {
    String buf;
    size_t pos;
    const HistoryEntry *search_pos;
    char *search_text;
    CompletionState completion;
} CommandLine;

extern const CommandSet cmd_mode_commands;
extern const CommandSet search_mode_commands;

void cmdline_set_text(CommandLine *c, const char *text) NONNULL_ARGS;
void cmdline_clear(CommandLine *c) NONNULL_ARGS;
void cmdline_free(CommandLine *c) NONNULL_ARGS;

#endif
