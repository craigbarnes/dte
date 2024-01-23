#ifndef STATUS_H
#define STATUS_H

#include <stddef.h>
#include "editor.h"
#include "mode.h"
#include "options.h"
#include "window.h"

size_t statusline_format_find_error(const char *str);

size_t sf_format (
    const Window *window,
    const GlobalOptions *opts,
    const ModeHandler *mode,
    char *buf,
    size_t size,
    const char *format
);

#endif
