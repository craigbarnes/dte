#include <string.h>
#include "xterm.h"
#include "output.h"
#include "../util/macros.h"

void xterm_save_title(void)
{
    term_add_literal("\033[22;2t");
}

void xterm_restore_title(void)
{
    term_add_literal("\033[23;2t");
}

void xterm_set_title(const char *title)
{
    term_add_literal("\033]2;");
    term_add_bytes(title, strlen(title));
    term_add_byte('\007');
}
