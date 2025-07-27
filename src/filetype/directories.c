static const struct DirPrefixMap {
    const char NONSTRING dir[15];
    uint8_t dir_len;
    FileTypeEnum filetype;
} prefixes[] = {
    {STRN("/etc/default/"), SH},
    {STRN("/etc/firejail/"), CONFIG},
    {STRN("/etc/nginx/"), NGINX},
    {STRN("/etc/pam.d/"), CONFIG},
    {STRN("/etc/sudoers.d/"), CONFIG},
};

UNITTEST {
    for (size_t i = 0; i < ARRAYLEN(prefixes); i++) {
        const struct DirPrefixMap *p = &prefixes[i];
        BUG_ON(p->dir_len < STRLEN("/a/b/"));
        BUG_ON(p->dir_len > sizeof(prefixes[0].dir));
        BUG_ON(p->dir[0] != '/');
        BUG_ON(p->dir[p->dir_len - 1] != '/');
        BUG_ON(p->filetype >= NR_BUILTIN_FILETYPES);
    }
}

static FileTypeEnum filetype_from_dir_prefix(StringView path)
{
    if (path.length < 5) {
        return NONE;
    }
    for (size_t i = 0; i < ARRAYLEN(prefixes); i++) {
        const struct DirPrefixMap *p = &prefixes[i];
        if (strview_has_sv_prefix(path, string_view(p->dir, p->dir_len))) {
            return p->filetype;
        }
    }
    return NONE;
}
