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

static FileTypeEnum filetype_from_emacs_var(const StringView line)
{
    const char *delim = xmemmem(line.data, MIN(line.length, 128), STRN("-*-"));
    if (!delim) {
        return NONE;
    }

    StringView vars = line;
    strview_remove_prefix(&vars, substr_len(vars.data, delim + STRLEN("-*-")));
    strview_trim_left(&vars);

    const char *end = xmemmem(vars.data, MIN(vars.length, 128), STRN("-*-"));
    if (!end) {
        return NONE;
    }

    vars.length = substr_len(vars.data, end);
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
        break; // <!--*-xml-*-->
    case '%':
        if (strview_has_prefix(&line, "%YAML")) {
            return YAML;
        }
        break; // % -*-latex-*-
    case '#':
        if (strview_has_prefix(&line, "#compdef ")) {
            return SH;
        } else if (strview_has_prefix(&line, "#autoload")) {
            return SH;
        }
        break; // # -*-shell-script-*-
    case 'd':
        return strview_has_prefix(&line, "diff --git") ? DIFF : NONE;
    case '/': // /* -*-c-*-
    case '.': // .. -*-rst-*-
    case ';': // ;; -*-scheme-*-
    case '-': // -- -*-lua-*-
    case '\\': // \input texinfo @c -*-texinfo-*-
        break;
    default:
        return NONE;
    }

    return filetype_from_emacs_var(line);
}
