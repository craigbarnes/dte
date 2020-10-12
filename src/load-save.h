#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H

#include <stdbool.h>
#include "buffer.h"
#include "encoding.h"

bool load_buffer(Buffer *b, bool must_exist, const char *filename);
bool save_buffer(Buffer *b, const char *filename, const Encoding *encoding, bool crlf, bool write_bom);
bool read_blocks(Buffer *b, int fd);

#endif
