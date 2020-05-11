#ifndef COMMAND_MACRO_H
#define COMMAND_MACRO_H

#include <stdbool.h>
#include <stddef.h>
#include "util/string.h"
#include "util/unicode.h"

bool macro_is_recording(void);
void macro_record(void);
void macro_stop(void);
void macro_toggle(void);
void macro_cancel(void);
void macro_play(void);
void macro_command_hook(const char *cmd_name, char **args);
void macro_insert_char_hook(CodePoint c);
void macro_insert_text_hook(const char *text, size_t size);
String dump_macro(void);

#endif
