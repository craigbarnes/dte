#include "cursed.h"

#include <curses.h>
#include <term.h>

int curses_init(const char *term)
{
	int err;

	if (setupterm((char *)term, 1, &err) == ERR) {
		// -1 (1) terminal is hardcopy
		// -2 (0) terminal could not be found
		// -3 (-1) terminfo database could not be found
		return err - 2;
	}
	return 0;
}

int curses_bool_cap(const char *name)
{
	return tigetflag((char *)name);
}

int curses_int_cap(const char *name)
{
	return tigetnum((char *)name);
}

char *curses_str_cap(const char *name)
{
	char *str = tigetstr((char *)name);

	if (str == (char *)-1) {
		// not a string cap (bug?)
		return NULL;
	}
	// NULL = canceled or absent
	return str;
}
