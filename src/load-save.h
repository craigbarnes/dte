#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H

#include <stdbool.h>
#include "buffer.h"
#include "encoding.h"
#include "options.h"
#include "util/macros.h"

bool load_buffer(Buffer *buffer, const char *filename, const GlobalOptions *gopts, bool must_exist) WARN_UNUSED_RESULT;
bool save_buffer(Buffer *buffer, const char *filename, const Encoding *encoding, bool crlf, bool write_bom, bool hardlinks) WARN_UNUSED_RESULT;
bool read_blocks(Buffer *buffer, int fd, bool utf8_bom) WARN_UNUSED_RESULT;

#endif
