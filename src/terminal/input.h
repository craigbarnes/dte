#ifndef TERMINAL_INPUT_H
#define TERMINAL_INPUT_H

#include "key.h"
#include "terminal.h"
#include "util/debug.h"
#include "util/macros.h"

static inline bool is_newly_detected_feature (
    TermFeatureFlags existing, // Existing Terminal::features bits
    TermFeatureFlags detected, // Return value from query.h function
    TermFeatureFlags feature   // Feature flag to test
) {
    BUG_ON(!IS_POWER_OF_2(feature));
    // (detected & feature) && !(existing & feature)
    return ~existing & detected & feature;
}

KeyCode term_read_input(Terminal *term, unsigned int esc_timeout_ms) NONNULL_ARGS;

#endif
