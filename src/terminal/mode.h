#ifndef TERMINAL_MODE_H
#define TERMINAL_MODE_H

#include <stdbool.h>
#include "util/macros.h"

bool term_mode_init(void) WARN_UNUSED_RESULT;
bool term_raw(void);
bool term_raw_isig(void);
bool term_cooked(void);

#endif
