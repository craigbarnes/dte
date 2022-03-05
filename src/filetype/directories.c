static const struct {
    const char dir[16];
    FileTypeEnum filetype;
} prefixes[] = {
    {"/etc/default/", SH},
    {"/etc/nginx/", NGINX},
    {"/etc/pam.d/", CONFIG},
    {"/etc/sudoers.d/", CONFIG},
};

UNITTEST {
    for (size_t i = 0; i < ARRAYLEN(prefixes); i++) {
        const char *dir = prefixes[i].dir;
        BUG_ON(dir[0] != '/');
        BUG_ON(dir[ARRAYLEN(prefixes[0].dir) - 1] != '\0');
        BUG_ON(dir[strlen(dir) - 1] != '/');
        BUG_ON(prefixes[i].filetype >= NR_BUILTIN_FILETYPES);
    }
}

static FileTypeEnum filetype_from_dir_prefix(StringView path)
{
    for (size_t i = 0; i < ARRAYLEN(prefixes); i++) {
        if (strview_has_prefix(&path, prefixes[i].dir)) {
            return prefixes[i].filetype;
        }
    }
    return NONE;
}
