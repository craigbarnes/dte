#ifndef ENCODING_ENCODING_H
#define ENCODING_ENCODING_H

#include "../util/macros.h"

typedef enum {
    UTF8,
    UTF16,
    UTF16BE,
    UTF16LE,
    UTF32,
    UTF32BE,
    UTF32LE,
    UNKNOWN_ENCODING,
    NR_ENCODING_TYPES,

    // This value is used by the "open" command to instruct other
    // routines that no specific encoding was requested and that
    // it should be detected instead. It is always replaced by
    // some other value by the time a file is successfully opened.
    ENCODING_AUTODETECT
} EncodingType;

typedef struct {
    EncodingType type;
    // An interned encoding name compatible with iconv_open(3)
    const char *name;
} Encoding;

Encoding encoding_from_type(EncodingType type);
Encoding encoding_from_name(const char *name) NONNULL_ARGS;
EncodingType lookup_encoding(const char *name) NONNULL_ARGS;

#endif
