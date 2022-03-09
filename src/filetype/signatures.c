static FileTypeEnum filetype_from_signature(const StringView line)
{
    if (line.length < 5) {
        return NONE;
    }

    switch (line.data[0]) {
    case '<':
        if (strview_has_prefix_icase(&line, "<!DOCTYPE HTML")) {
            return HTML;
        } else if (strview_has_prefix(&line, "<!DOCTYPE")) {
            return XML;
        } else if (strview_has_prefix(&line, "<?xml")) {
            return XML;
        }
        break;
    case '%':
        if (strview_has_prefix(&line, "%YAML")) {
            return YAML;
        }
        break;
    case 'd':
        if (strview_has_prefix(&line, "diff --git")) {
            return DIFF;
        }
        break;
    }

    return NONE;
}
