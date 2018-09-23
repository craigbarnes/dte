#ifndef SCRIPT_H
#define SCRIPT_H

typedef enum {
    HOOK_SET_FILE_OPTIONS,
    NR_EVENT_HOOK_TYPES
} EventHookType;

void script_state_init(void);
void run_builtin_script(const char *name);
void run_event_hooks(EventHookType event_type);
void cmd_lua(const char* pf, char **args);
void cmd_lua_file(const char* pf, char **args);

#endif
