#ifndef DETECT_H
#define DETECT_H

#include "libc.h"

struct buffer;

char *detect_interpreter(struct buffer *b);
bool detect_indent(struct buffer *b);

#endif
