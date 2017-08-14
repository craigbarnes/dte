#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H

#include "buffer.h"

int load_buffer(Buffer *b, bool must_exist, const char *filename);
int save_buffer(Buffer *b, const char *filename, const char *encoding, LineEndingType newline);

#endif
