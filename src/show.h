#ifndef SHOW_H
#define SHOW_H

#include <stdbool.h>
#include "editor.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

typedef enum {
    SHOW_DTERC = 0x1, // Use "dte" filetype (and syntax highlighter)
    SHOW_LASTLINE = 0x2, // Move cursor to last line (e.g. most recent history entry)
    SHOW_MSGLINE = 0x4, // Move cursor to line containing current message
} ShowHandlerFlags;

typedef struct {
    const char name[11];
    uint8_t flags; // ShowHandlerFlags
    String (*dump)(EditorState *e);
    bool (*show)(EditorState *e, const char *name, bool cmdline);
    void (*complete_arg)(EditorState *e, PointerArray *a, const char *prefix);
} ShowHandler;

bool show(EditorState *e, const char *type, const char *key, bool cflag) NONNULL_ARG(1, 2) WARN_UNUSED_RESULT;
void collect_show_subcommands(PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_show_subcommand_args(EditorState *e, PointerArray *a, const char *name, const char *arg_prefix) NONNULL_ARGS;
const ShowHandler *lookup_show_handler(const char *name) NONNULL_ARGS;
String dump_command_history(EditorState *e) NONNULL_ARGS;
String dump_search_history(EditorState *e) NONNULL_ARGS;

#endif
