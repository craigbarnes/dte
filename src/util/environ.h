#ifndef UTIL_ENVIRON_H
#define UTIL_ENVIRON_H

#include "macros.h"

// The glibc <unistd.h> header declares `environ` in some cases (if
// _GNU_SOURCE is defined before inclusion), whereas it needs to be
// declared explicitly in others. The former can happen in some parts
// of this codebase, due to the generated build/gen/build-defs.h header
// being included. Since identical re-declarations are well defined
// in C, we simply silence any -Wredundant-decls warnings and declare
// it unconditionally.

IGNORE_WARNING("-Wredundant-decls")
extern char **environ; // NOLINT(*-avoid-non-const-global-variables)
UNIGNORE_WARNINGS

#endif
