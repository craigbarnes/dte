#include <stddef.h>
#include <strings.h>
#include "encoding.h"
#include "../common.h"
#include "../util/ascii.h"
#include "../util/macros.h"

static const char encoding_names[][16] = {
    [UTF8] = "UTF-8",
    [UTF16] = "UTF-16",
    [UTF16BE] = "UTF-16BE",
    [UTF16LE] = "UTF-16LE",
    [UTF32] = "UTF-32",
    [UTF32BE] = "UTF-32BE",
    [UTF32LE] = "UTF-32LE",
};

static_assert(ARRAY_COUNT(encoding_names) == NR_ENCODING_TYPES - 1);

static const struct {
    const char alias[8];
    EncodingType encoding;
} encoding_aliases[] = {
    {"UTF8", UTF8},
    {"UTF16", UTF16},
    {"UTF16BE", UTF16BE},
    {"UTF16LE", UTF16LE},
    {"UTF32", UTF32},
    {"UTF32BE", UTF32BE},
    {"UTF32LE", UTF32LE},
    {"UCS2", UTF16},
    {"UCS-2", UTF16},
    {"UCS-2BE", UTF16BE},
    {"UCS-2LE", UTF16LE},
    {"UCS4", UTF32},
    {"UCS-4", UTF32},
    {"UCS-4BE", UTF32BE},
    {"UCS-4LE", UTF32LE},
};

EncodingType lookup_encoding(const char *name)
{
    for (size_t i = 0; i < ARRAY_COUNT(encoding_names); i++) {
        if (strcasecmp(name, encoding_names[i]) == 0) {
            return (EncodingType) i;
        }
    }
    for (size_t i = 0; i < ARRAY_COUNT(encoding_aliases); i++) {
        if (strcasecmp(name, encoding_aliases[i].alias) == 0) {
            return encoding_aliases[i].encoding;
        }
    }
    return UNKNOWN_ENCODING;
}

const char *encoding_type_to_string(EncodingType type)
{
    if (type < NR_ENCODING_TYPES && type != UNKNOWN_ENCODING) {
        return encoding_names[type];
    }
    return NULL;
}
