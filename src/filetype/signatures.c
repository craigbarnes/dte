typedef struct {
    const char bytes[11];
    uint8_t filetype; // FileTypeEnum
} FileSignatureMap;

static const FileSignatureMap signatures[] = {
    {"<!DOCTYPE", XML},
    {"<?xml", XML},
    {"%YAML", YAML},
    {"diff --git", DIFF},
};

static FileTypeEnum filetype_from_signature(const StringView line)
{
    if (line.length < 5) {
        return NONE;
    }

    switch (line.data[0]) {
    case '<':
    case '%':
    case 'd':
        break;
    default:
        return NONE;
    }

    if (strview_has_prefix_icase(&line, "<!DOCTYPE HTML")) {
        return HTML;
    }

    for (size_t i = 0; i < ARRAY_COUNT(signatures); i++) {
        if (strview_has_prefix(&line, signatures[i].bytes)) {
            return signatures[i].filetype;
        }
    }

    return NONE;
}
