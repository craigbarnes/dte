#ifndef ENCODING_H
#define ENCODING_H

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
} EncodingType;

typedef struct {
    unsigned char bytes[4];
    unsigned int len;
} ByteOrderMark;

EncodingType lookup_encoding(const char *name) NONNULL_ARGS;

static inline bool encoding_is_utf8(const char *name)
{
    return lookup_encoding(name) == UTF8;
}

static inline bool encoding_type_has_bom(EncodingType type)
{
    return (type >= UTF8 && type <= UTF32LE);
}

const char *encoding_normalize(const char *name) NONNULL_ARGS_AND_RETURN;
const char *encoding_from_type(EncodingType type) RETURNS_NONNULL;
EncodingType detect_encoding_from_bom(const char *buf, size_t size);
const ByteOrderMark *get_bom_for_encoding(EncodingType type);

#endif
