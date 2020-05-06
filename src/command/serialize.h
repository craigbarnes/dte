#ifndef COMMAND_SERIALIZE_H
#define COMMAND_SERIALIZE_H

#include <stdbool.h>
#include "util/string.h"
#include "util/string-view.h"

char *escape_command_arg(const char *arg, bool escape_tilde);
void string_append_escaped_arg_sv(String *s, StringView arg, bool escape_tilde);

static inline void string_append_escaped_arg(String *s, const char *arg, bool escape_tilde)
{
    return string_append_escaped_arg_sv(s, strview_from_cstring(arg), escape_tilde);
}

#endif
