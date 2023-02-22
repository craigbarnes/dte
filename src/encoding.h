#ifndef ENCODING_ENCODING_H
#define ENCODING_ENCODING_H

#include <stdbool.h>
#include <stddef.h>
#include "util/macros.h"

typedef enum {
    UTF8,
    UTF16BE,
    UTF16LE,
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

typedef struct {
    const unsigned char bytes[4];
    unsigned int len;
} ByteOrderMark;

static inline bool same_encoding(const Encoding *a, const Encoding *b)
{
    return a->type == b->type && a->name == b->name;
}

Encoding encoding_from_type(EncodingType type);
Encoding encoding_from_name(const char *name) NONNULL_ARGS;
EncodingType lookup_encoding(const char *name) NONNULL_ARGS;
EncodingType detect_encoding_from_bom(const unsigned char *buf, size_t size);
const ByteOrderMark *get_bom_for_encoding(EncodingType encoding);

#endif
