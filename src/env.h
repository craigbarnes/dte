#ifndef ENV_H
#define ENV_H

void collect_builtin_env(const char *prefix);
char *expand_builtin_env(const char *name);

#endif
