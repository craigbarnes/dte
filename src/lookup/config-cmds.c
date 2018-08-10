static bool lookup_config_command(const char *s, size_t len)
{
    switch(len) {
    case 2:
        switch(s[0]) {
        case 'c':
            return (s[1] != 'd') ? false : true;
        case 'f':
            return (s[1] != 't') ? false : true;
        case 'h':
            return (s[1] != 'i') ? false : true;
        }
        return false;
    case 3:
        return memcmp(s, "set", 3) ? false : true;
    case 4:
        return memcmp(s, "bind", 4) ? false : true;
    case 5:
        return memcmp(s, "alias", 5) ? false : true;
    case 6:
        switch(s[0]) {
        case 'o':
            return memcmp(s + 1, "ption", 5) ? false : true;
        case 's':
            return memcmp(s + 1, "etenv", 5) ? false : true;
        }
        return false;
    case 7:
        return memcmp(s, "include", 7) ? false : true;
    case 8:
        return memcmp(s, "errorfmt", 8) ? false : true;
    case 11:
        return memcmp(s, "load-syntax", 11) ? false : true;
    }
    return false;
}
