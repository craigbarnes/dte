#ifndef COMMAND_MACRO_H
#define COMMAND_MACRO_H

#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"
#include "util/unicode.h"

typedef struct {
    PointerArray macro;
    PointerArray prev_macro;
    String insert_buffer;
    bool recording;
} CommandMacroState;

static inline bool macro_is_recording(const CommandMacroState *m)
{
    return m->recording;
}

bool macro_record(CommandMacroState *m) WARN_UNUSED_RESULT NONNULL_ARGS;
bool macro_stop(CommandMacroState *m) WARN_UNUSED_RESULT NONNULL_ARGS;
bool macro_cancel(CommandMacroState *m) WARN_UNUSED_RESULT NONNULL_ARGS;
void macro_command_hook(CommandMacroState *m, const char *cmd_name, char **args) NONNULL_ARGS;
void macro_search_hook(CommandMacroState *m, const char *pattern, bool reverse, bool add_to_history) NONNULL_ARG(1);
void macro_insert_char_hook(CommandMacroState *m, CodePoint c) NONNULL_ARGS;
void macro_insert_text_hook(CommandMacroState *m, const char *text, size_t size) NONNULL_ARG(1);
String dump_macro(const CommandMacroState *m) WARN_UNUSED_RESULT NONNULL_ARGS;
void free_macro(CommandMacroState *m) NONNULL_ARGS;

#endif
