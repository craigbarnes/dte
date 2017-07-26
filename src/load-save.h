#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H

#include "buffer.h"

int load_buffer(struct buffer *b, bool must_exist, const char *filename);
int save_buffer(struct buffer *b, const char *filename, const char *encoding, enum newline_sequence newline);

#endif
