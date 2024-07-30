#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

// NOLINTNEXTLINE(*-non-const-global-variables)
extern volatile sig_atomic_t resized;

void set_basic_signal_dispositions(void);
void set_fatal_signal_handlers(void);
void set_sigwinch_handler(void);

#endif
