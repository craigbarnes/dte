#ifndef STATUS_H
#define STATUS_H

#include <stddef.h>
#include "editor.h"
#include "options.h"
#include "window.h"

size_t statusline_format_find_error(const char *str);

void sf_format (
    const Window *window,
    const GlobalOptions *opts,
    InputMode mode,
    char *buf,
    size_t size,
    const char *format
);

#endif
