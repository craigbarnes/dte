#ifndef COMMAND_SERIALIZE_H
#define COMMAND_SERIALIZE_H

#include <stdbool.h>
#include "util/macros.h"
#include "util/string.h"
#include "util/string-view.h"

void string_append_escaped_arg_sv(String *s, StringView arg, bool escape_tilde) NONNULL_ARGS;

static inline void string_append_escaped_arg(String *s, const char *arg, bool escape_tilde)
{
    string_append_escaped_arg_sv(s, strview(arg), escape_tilde);
}

#endif
