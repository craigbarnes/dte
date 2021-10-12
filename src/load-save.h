#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H

#include <stdbool.h>
#include "buffer.h"
#include "encoding.h"
#include "util/macros.h"

bool load_buffer(Buffer *b, bool must_exist, const char *filename) WARN_UNUSED_RESULT;
bool save_buffer(Buffer *b, const char *filename, const Encoding *encoding, bool crlf, bool write_bom) WARN_UNUSED_RESULT;
bool read_blocks(Buffer *b, int fd) WARN_UNUSED_RESULT;

#endif
