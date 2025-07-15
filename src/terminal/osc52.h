#ifndef TERMINAL_OSC52_H
#define TERMINAL_OSC52_H

#include <stdbool.h>
#include <stddef.h>
#include "terminal.h"
#include "util/macros.h"
#include "util/string-view.h"

typedef enum {
    TCOPY_CLIPBOARD = 1 << 0,
    TCOPY_PRIMARY = 1 << 1,
} TermCopyFlags;

bool term_osc52_copy(TermOutputBuffer *output, StringView text, TermCopyFlags flags) NONNULL_ARGS WARN_UNUSED_RESULT;

#endif
