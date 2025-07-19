#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>
#include "util/errorcode.h"

extern volatile sig_atomic_t resized; // NOLINT(*-non-const-global-variables)

void set_basic_signal_dispositions(void);
SystemErrno set_fatal_signal_handlers(void);
SystemErrno set_sigwinch_handler(void);
void set_signal_dispositions_headless(void);

#endif
