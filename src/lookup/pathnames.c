static FileTypeEnum filetype_from_pathname(const char *buf, size_t len)
{
    switch(len) {
    case 10:
        if (memcmp(buf, "/etc/", 5)) {
            return 0;
        }
        switch(buf[5]) {
        case 'f':
            return memcmp(buf + 6, "stab", 4) ? 0 : CONFIG;
        case 'h':
            return memcmp(buf + 6, "osts", 4) ? 0 : CONFIG;
        }
        return 0;
    case 19:
        return memcmp(buf, "/boot/grub/menu.lst", 19) ? 0 : CONFIG;
    }
    return 0;
}
