#ifndef TERMINAL_COPY_PASTE_H
#define TERMINAL_COPY_PASTE_H

#include <stdbool.h>
#include <stddef.h>

bool osc52_copy(const char *text, size_t text_len, bool clipboard, bool primary);
bool kitty_osc52_copy(const char *text, size_t text_len, bool clipboard, bool primary);

#endif
