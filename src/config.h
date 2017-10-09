#ifndef CONFIG_H
#define CONFIG_H

#include "command.h"
#include "libc.h"

extern const char *config_file;
extern int config_line;

void exec_config(const Command *cmds, const char *buf, size_t size);
int do_read_config(const Command *cmds, const char *filename, bool must_exist);
int read_config(const Command *cmds, const char *filename, bool must_exist);
void exec_builtin_rc(const char *rc);
void exec_reset_colors_rc(void);

#endif
