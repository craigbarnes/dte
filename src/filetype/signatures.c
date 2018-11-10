typedef struct {
    const unsigned char NONSTRING bytes[11];
    uint8_t length;
    FileTypeEnum filetype;
} FileSignatureMap;

#define SIG(str, ft) { \
    .bytes = str, \
    .length = STRLEN(str), \
    .filetype = ft \
}

static const FileSignatureMap signatures[] = {
    SIG("<!DOCTYPE", XML),
    SIG("<?xml", XML),
    SIG("%YAML", YAML),
    SIG("[wrap-file]", INI),
    SIG("diff --git", DIFF),
};

static PURE FileTypeEnum filetype_from_signature(const char *s, size_t len)
{
    if (len < 5) {
        return NONE;
    }

    switch (s[0]) {
    case '<':
    case '%':
    case '[':
    case 'd':
        break;
    default:
        return NONE;
    }

    if (len >= 14 && strncasecmp(s, "<!DOCTYPE HTML", 14) == 0) {
        return HTML;
    }

    for (size_t i = 0; i < ARRAY_COUNT(signatures); i++) {
        const FileSignatureMap *sig = &signatures[i];
        const size_t sig_length = sig->length;
        if (len >= sig_length && memcmp(s, sig->bytes, sig_length) == 0) {
            return sig->filetype;
        }
    }

    return NONE;
}
