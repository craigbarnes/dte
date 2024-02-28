#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

// NOLINTNEXTLINE(*-non-const-global-variables)
extern volatile sig_atomic_t resized;

void set_signal_handlers(void);

#endif
