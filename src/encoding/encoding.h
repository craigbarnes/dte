#ifndef ENCODING_ENCODING_H
#define ENCODING_ENCODING_H

typedef enum {
    UTF8,
    UTF16,
    UTF16BE,
    UTF16LE,
    UTF32,
    UTF32BE,
    UTF32LE,
    UNKNOWN_ENCODING,
    NR_ENCODING_TYPES
} EncodingType;

EncodingType lookup_encoding(const char *name);
const char *encoding_type_to_string(EncodingType type);

#endif
