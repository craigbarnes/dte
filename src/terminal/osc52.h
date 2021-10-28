#ifndef TERMINAL_OSC52_H
#define TERMINAL_OSC52_H

#include <stdbool.h>
#include <stddef.h>
#include "output.h"

bool osc52_copy(TermOutputBuffer *output, const char *text, size_t text_len, bool clipboard, bool primary);

#endif
