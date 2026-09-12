#ifndef COMMAND_MACRO_H
#define COMMAND_MACRO_H

#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/string.h"
#include "util/unicode.h"
#include "view.h"

typedef struct {
    PointerArray macro;
    PointerArray prev_macro;
    String insert_buffer;
    bool recording;
} MacroRecorder;

static inline bool macro_is_recording(const MacroRecorder *m)
{
    return m->recording;
}

bool macro_record(MacroRecorder *m) WARN_UNUSED_RESULT NONNULL_ARGS;
bool macro_stop(MacroRecorder *m) WARN_UNUSED_RESULT NONNULL_ARGS;
bool macro_cancel(MacroRecorder *m) WARN_UNUSED_RESULT NONNULL_ARGS;
void macro_command_hook(MacroRecorder *m, const char *cmd_name, char **args) NONNULL_ARGS;
void macro_search_hook(MacroRecorder *m, StringView pattern, SelectionType sel_type, bool reverse, bool add_to_history) NONNULL_ARGS;
void macro_insert_char_hook(MacroRecorder *m, CodePoint c) NONNULL_ARGS;
void macro_insert_text_hook(MacroRecorder *m, StringView text) NONNULL_ARGS;
String dump_macro(const MacroRecorder *m) WARN_UNUSED_RESULT NONNULL_ARGS;
void free_macro(MacroRecorder *m) NONNULL_ARGS;

#endif
