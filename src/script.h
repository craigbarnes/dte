#ifndef SCRIPT_H
#define SCRIPT_H

void script_state_init(void);
void run_builtin_script(const char *name);
void cmd_lua(const char* pf, char **args);
void cmd_lua_file(const char* pf, char **args);

#endif
