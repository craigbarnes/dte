#ifndef EDITOR_H
#define EDITOR_H

#include "libc.h"

enum editor_status {
	EDITOR_INITIALIZING,
	EDITOR_RUNNING,
	EDITOR_EXITING,
};

enum input_mode {
	INPUT_NORMAL,
	INPUT_COMMAND,
	INPUT_SEARCH,
	INPUT_GIT_OPEN,
};

extern enum editor_status editor_status;
extern enum input_mode input_mode;
extern struct cmdline cmdline;
extern char *home_dir;
extern char *charset;
extern bool child_controls_terminal;
extern bool resized;
extern int cmdline_x;

extern const char *program;
extern const char *version;
extern const char *pkgdatadir;

char *editor_file(const char *name);
char get_confirmation(const char *choices, const char *format, ...) FORMAT(2);
void set_input_mode(enum input_mode mode);
void any_key(void);
void normal_update(void);
void resize(void);
void ui_end(void);
void suspend(void);
void set_signal_handler(int signum, void (*handler)(int));
void main_loop(void);

#endif
