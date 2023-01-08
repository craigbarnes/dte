#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

extern volatile sig_atomic_t resized;

void set_signal_handlers(void);
void handle_sigwinch(int signum);

#endif
