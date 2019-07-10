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
    SIG("[core]", INI), // .git/config file
    SIG("[wrap-file]", INI), // Meson wrap file
    SIG("[Trigger]", INI), // libalpm hook
    SIG("diff --git", DIFF),
};

static FileTypeEnum filetype_from_signature(const StringView sv)
{
    if (sv.length < 5) {
        return NONE;
    }

    switch (sv.data[0]) {
    case '<':
    case '%':
    case '[':
    case 'd':
        break;
    default:
        return NONE;
    }

    if (string_view_has_literal_prefix_icase(&sv, "<!DOCTYPE HTML")) {
        return HTML;
    }

    for (size_t i = 0; i < ARRAY_COUNT(signatures); i++) {
        const FileSignatureMap *sig = &signatures[i];
        if (string_view_has_prefix(&sv, sig->bytes, sig->length)) {
            return sig->filetype;
        }
    }

    return NONE;
}
