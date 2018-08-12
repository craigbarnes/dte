static FileTypeEnum filetype_from_pathname(const char *s, size_t len)
{
    switch (len) {
    case 10:
        if (memcmp(s, "/etc/", 5)) {
            return 0;
        }
        switch (s[5]) {
        case 'f':
            return memcmp(s + 6, "stab", 4) ? 0 : CONFIG;
        case 'h':
            return memcmp(s + 6, "osts", 4) ? 0 : CONFIG;
        }
        return 0;
    case 19:
        return memcmp(s, "/boot/grub/menu.lst", 19) ? 0 : CONFIG;
    }
    return 0;
}
