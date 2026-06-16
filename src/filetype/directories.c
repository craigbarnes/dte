static const struct DirPrefixMap {
    StringView prefix;
    StringView suffix;
    FileTypeEnum filetype;
} affixes[] = {
    {SV("/etc/default/"), SV(""), SH},
    {SV("/etc/firejail/"), SV(""), CONFIG},
    {SV("/etc/nginx/"), SV(""), NGINX},
    {SV("/etc/pam.d/"), SV(""), CONFIG},
    {SV("/etc/sudoers.d/"), SV(""), CONFIG},
    {SV("/etc/systemd/"), SV(".conf"), INI},
    {SV("/etc/"), SV(".conf"), CONFIG},
    {SV("/usr/share/"), SV(".conf"), CONFIG},
    {SV("/usr/local/share/"), SV(".conf"), CONFIG},
};

UNITTEST {
    for (size_t i = 0; i < ARRAYLEN(affixes); i++) {
        const struct DirPrefixMap *p = &affixes[i];
        BUG_ON(p->prefix.length < STRLEN("/etc/"));
        BUG_ON(!strview_has_prefix(p->prefix, "/"));
        BUG_ON(!strview_has_suffix(p->prefix, "/"));
        BUG_ON(p->suffix.length && !strview_has_prefix(p->suffix, "."));
        BUG_ON(p->filetype >= NR_BUILTIN_FILETYPES);
    }
}

static FileTypeEnum filetype_from_path(StringView path)
{
    if (path.length < STRLEN("/etc/_")) {
        return NONE;
    }

    for (size_t i = 0; i < ARRAYLEN(affixes); i++) {
        const struct DirPrefixMap *p = &affixes[i];
        if (strview_has_sv_prefix_and_suffix(path, p->prefix, p->suffix)) {
            return p->filetype;
        }
    }

    return NONE;
}
