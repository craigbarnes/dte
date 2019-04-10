#ifndef ENCODING_ENCODING_H
#define ENCODING_ENCODING_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../util/xmalloc.h"

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
    // If type is UNKNOWN_ENCODING, this is an encoding name compatible
    // with iconv_open(3). Otherwise it's NULL and the name string can
    // obtained using encoding_type_to_string(type).
    char *name;
} Encoding;

EncodingType lookup_encoding(const char *name) NONNULL_ARGS;
const char *encoding_type_to_string(EncodingType type);

NONNULL_ARGS
static inline Encoding encoding_from_name(const char *name)
{
    const EncodingType type = lookup_encoding(name);
    Encoding e = {
        .type = type,
        .name = (type == UNKNOWN_ENCODING) ? xstrdup_toupper(name) : NULL
    };
    return e;
}

NONNULL_ARGS
static inline const char *encoding_to_string(const Encoding *e)
{
    if (e->type == UNKNOWN_ENCODING) {
        return e->name;
    } else {
        return encoding_type_to_string(e->type);
    }
}

NONNULL_ARGS
static inline Encoding encoding_clone(const Encoding *e)
{
    Encoding clone = {
        .type = e->type,
        .name = (e->type == UNKNOWN_ENCODING) ? xstrdup(e->name) : NULL
    };
    return clone;
}

static inline void free_encoding(Encoding *e)
{
    free(e->name);
}

#endif
