static const struct {
    const char *const key;
    const FileTypeEnum val;
} filetype_from_pathname_table[3] = {
    {"/boot/grub/menu.lst", CONFIG},
    {"/etc/fstab", CONFIG},
    {"/etc/hosts", CONFIG},
};

#define CMP(i) idx = i; goto compare

static FileTypeEnum filetype_from_pathname(const char *s, size_t len)
{
    size_t idx;
    const char *key;
    FileTypeEnum val;
    switch (len) {
    case 10:
        if (memcmp(s, "/etc/", 5)) {
            return 0;
        }
        switch (s[5]) {
        case 'f': CMP(1); // /etc/fstab
        case 'h': CMP(2); // /etc/hosts
        }
        break;
    case 19: CMP(0); // /boot/grub/menu.lst
    }
    return 0;
compare:
    key = filetype_from_pathname_table[idx].key;
    val = filetype_from_pathname_table[idx].val;
    return memcmp(s, key, len) ? 0 : val;
}

#undef CMP
