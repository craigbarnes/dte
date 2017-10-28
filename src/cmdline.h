#ifndef CMDLINE_H
#define CMDLINE_H

#include <stddef.h>
#include "ptr-array.h"
#include "strbuf.h"
#include "key.h"
#include "term.h"

typedef struct {
    StringBuffer buf;
    size_t pos;
    int search_pos;
    char *search_text;
} CommandLine;

enum {
    CMDLINE_UNKNOWN_KEY,
    CMDLINE_KEY_HANDLED,
    CMDLINE_CANCEL,
};

#define CMDLINE_INIT { \
    .buf = STRBUF_INIT, \
    .pos = 0, \
    .search_pos = -1, \
    .search_text = NULL \
}

void cmdline_clear(CommandLine *c);
void cmdline_set_text(CommandLine *c, const char *text);
int cmdline_handle_key(CommandLine *c, PointerArray *history, Key key);

#endif
