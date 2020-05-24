typedef struct {
    const unsigned char NONSTRING bytes[11];
    uint8_t length;
    FileTypeEnum filetype;
} FileSignatureMap;

static const FileSignatureMap signatures[] = {
    {STRN("<!DOCTYPE"), XML},
    {STRN("<?xml"), XML},
    {STRN("%YAML"), YAML},
    {STRN("[core]"), INI}, // .git/config file
    {STRN("[wrap-file]"), INI}, // Meson wrap file
    {STRN("[Trigger]"), INI}, // libalpm hook
    {STRN("diff --git"), DIFF},
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

    if (strview_has_prefix_icase(&sv, "<!DOCTYPE HTML")) {
        return HTML;
    }

    for (size_t i = 0; i < ARRAY_COUNT(signatures); i++) {
        const FileSignatureMap *sig = &signatures[i];
        const size_t len = sig->length;
        if (sv.length >= len && mem_equal(sv.data, sig->bytes, len)) {
            return sig->filetype;
        }
    }

    return NONE;
}
