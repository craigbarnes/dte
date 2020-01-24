#ifndef TERMINAL_MODE_H
#define TERMINAL_MODE_H

#include <stdbool.h>

bool term_mode_init(void);
void term_raw(void);
void term_raw_isig(void);
void term_cooked(void);

#endif
