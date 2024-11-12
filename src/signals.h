#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

extern volatile sig_atomic_t resized; // NOLINT(*-non-const-global-variables)

void set_basic_signal_dispositions(void);
void set_fatal_signal_handlers(void);
void set_sigwinch_handler(void);
void set_signal_dispositions_headless(void);

#endif
