static const struct {
    const char *const key;
    const FileTypeEnum val;
} filetype_from_pathname_table[3] = {
    {"/boot/grub/menu.lst", CONFIG},
    {"/etc/fstab", CONFIG},
    {"/etc/hosts", CONFIG},
};

static FileTypeEnum filetype_from_pathname(const char *s, size_t len)
{
    size_t idx;
    const char *key;
    FileTypeEnum val;
    switch (len) {
    case 10:
        switch (s[5]) {
        case 'f':
            idx = 1; // /etc/fstab
            goto compare;
        case 'h':
            idx = 2; // /etc/hosts
            goto compare;
        }
        break;
    case 19:
        idx = 0; // /boot/grub/menu.lst
        goto compare;
    }
    return 0;
compare:
    key = filetype_from_pathname_table[idx].key;
    val = filetype_from_pathname_table[idx].val;
    return memcmp(s, key, len) ? 0 : val;
}
