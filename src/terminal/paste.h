#ifndef TERMINAL_PASTE_H
#define TERMINAL_PASTE_H

#include <stdbool.h>
#include "terminal.h"
#include "util/macros.h"
#include "util/string.h"

String term_read_paste(TermInputBuffer *input, bool bracketed) NONNULL_ARGS;
void term_discard_paste(TermInputBuffer *input, bool bracketed) NONNULL_ARGS;

#endif
