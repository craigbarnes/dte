#ifndef CURSED_H
#define CURSED_H

int curses_init(const char *term);
int curses_bool_cap(const char *name);
int curses_int_cap(const char *name);
char *curses_str_cap(const char *name);

#endif
