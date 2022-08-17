#ifndef COMMAND_MACRO_H
#define COMMAND_MACRO_H

#include <stdbool.h>
#include <stddef.h>
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

void macro_record(CommandMacroState *m);
void macro_stop(CommandMacroState *m);
void macro_toggle(CommandMacroState *m);
void macro_cancel(CommandMacroState *m);
void macro_play(CommandMacroState *m);
void macro_command_hook(CommandMacroState *m, const char *cmd_name, char **args);
void macro_insert_char_hook(CommandMacroState *m, CodePoint c);
void macro_insert_text_hook(CommandMacroState *m, const char *text, size_t size);
String dump_macro(const CommandMacroState *m);
void free_macro(CommandMacroState *m);

#endif
