#ifndef CMDLINE_H
#define CMDLINE_H

#include <sys/types.h>
#include "terminal/key.h"
#include "util/ptr-array.h"
#include "util/string.h"

typedef struct {
    String buf;
    size_t pos;
    ssize_t search_pos;
    char *search_text;
} CommandLine;

typedef enum {
    CMDLINE_UNKNOWN_KEY,
    CMDLINE_KEY_HANDLED,
    CMDLINE_CANCEL,
} CommandLineResult;

void cmdline_clear(CommandLine *c);
void cmdline_set_text(CommandLine *c, const char *text);

CommandLineResult cmdline_handle_key (
    CommandLine *c,
    PointerArray *history,
    KeyCode key
);

#endif
