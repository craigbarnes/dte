static const struct EmacsModeEntry {
    char name[15];
    uint8_t type; // FileTypeEnum
} emacs_modes[] = {
    // Strings already in builtin_filetype_names[] need not be included here
    {"autoconf", M4},
    {"c++", CPLUSPLUS},
    {"cperl", PERL},
    {"latex", TEX},
    {"makefile", MAKE},
    {"muttrc", CONFIG},
    {"nroff", ROFF},
    {"nxml", XML},
    {"shell-script", SH},
};

static ssize_t find_var_delim(const char *start, size_t len)
{
    const char *delim = xmemmem(start, len, STRN("-*-"));
    return delim ? delim - start : -1;
}

static FileTypeEnum filetype_from_emacs_var(const StringView line)
{
    ssize_t delim_offset = find_var_delim(line.data, MIN(line.length, 128));
    if (delim_offset < 0) {
        return NONE;
    }

    StringView vars = line;
    strview_remove_prefix(&vars, delim_offset + STRLEN("-*-"));
    strview_trim_left(&vars);

    delim_offset = find_var_delim(vars.data, MIN(vars.length, 128));
    if (delim_offset <= 0) {
        return NONE;
    }

    vars.length = delim_offset;
    strview_trim_right(&vars);
    LOG_DEBUG("found emacs file-local vars: '%.*s'", (int)vars.length, vars.data);

    // TODO: Handle multiple local vars like e.g. "mode: example; other: xyz;",
    // in addition to a single major mode string (by searching for "[Mm]ode:")?
    if (vars.length > FILETYPE_NAME_MAX) {
        return NONE;
    }

    // Write the extracted var string into a buffer, to null-terminate and
    // convert to lowercase (making the lookup effectively case-insensitive)
    char name[FILETYPE_NAME_MAX + 1];
    name[vars.length] = '\0';
    for (size_t i = 0; i < vars.length; i++) {
        name[i] = ascii_tolower(vars.data[i]);
    }

    // TODO: Remove "-ts" (tree-sitter), "-base" and "-ts-base" suffixes
    // from name, before lookup?
    const struct EmacsModeEntry *entry = BSEARCH(name, emacs_modes, vstrcmp);
    if (entry) {
        return entry->type;
    }

    ssize_t ft = BSEARCH_IDX(name, builtin_filetype_names, vstrcmp);
    return (ft >= 0) ? (FileTypeEnum)ft : NONE;
}

static bool is_gitlog_commit_line(StringView line)
{
    if (!strview_remove_matching_prefix(&line, "commit ") || line.length < 40) {
        return false;
    }

    size_t ndigits = ascii_hex_prefix_length(line.data, line.length);
    if (ndigits < 40) {
        return false;
    }

    strview_remove_prefix(&line, ndigits);
    return line.length == 0 || strview_has_prefix_and_suffix(&line, " (", ")");
}

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
    case '#':
        if (strview_has_prefix(&line, "#compdef ")) {
            return SH;
        } else if (strview_has_prefix(&line, "#autoload")) {
            return SH;
        }
        break;
    case 'I':
        return strview_has_prefix(&line, "ISO-10303-21;") ? STEP : NONE;
    case 'c':
        return is_gitlog_commit_line(line) ? GITLOG : NONE;
    case 'd':
        return strview_has_prefix(&line, "diff --git") ? DIFF : NONE;
    case 's':
        return strview_has_prefix(&line, "stash@{") ? GITSTASH : NONE;
    case '/':
    case '.':
    case ';':
    case '-':
    case '\\':
        // See: "emacs style file-local variables" in test_find_ft_firstline()
        break;
    default:
        return NONE;
    }

    return filetype_from_emacs_var(line);
}
