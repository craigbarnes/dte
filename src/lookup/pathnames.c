static const struct {
    const char *const key;
    const FileTypeEnum val;
} filetype_from_pathname_table[3] = {
    {"/boot/grub/menu.lst", CONFIG},
    {"/etc/fstab", CONFIG},
    {"/etc/hosts", CONFIG},
};

#define CMP(i) idx = i; goto compare
#define CMPN(i) idx = i; goto compare_last_char
#define KEY filetype_from_pathname_table[idx].key
#define VAL filetype_from_pathname_table[idx].val

static FileTypeEnum filetype_from_pathname(const char *s, size_t len)
{
    size_t idx;
    switch (len) {
    case 10:
        switch (s[5]) {
        case 'f': CMP(1); // /etc/fstab
        case 'h': CMP(2); // /etc/hosts
        }
        break;
    case 19: CMP(0); // /boot/grub/menu.lst
    }
    return 0;
compare:
    return (memcmp(s, KEY, len) == 0) ? VAL : 0;
compare_last_char:
    return (s[len - 1] == KEY[len - 1]) ? VAL : 0;
}

#undef CMP
#undef CMPN
#undef KEY
#undef VAL
