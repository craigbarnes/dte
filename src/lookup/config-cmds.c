static bool lookup_config_command(const char *buf, size_t len)
{
    switch(len) {
    case 2:
        switch(buf[0]) {
        case 'c':
            return (buf[1] != 'd') ? false : true;
        case 'f':
            return (buf[1] != 't') ? false : true;
        case 'h':
            return (buf[1] != 'i') ? false : true;
        }
        return false;
    case 3:
        return memcmp(buf, "set", 3) ? false : true;
    case 4:
        return memcmp(buf, "bind", 4) ? false : true;
    case 5:
        return memcmp(buf, "alias", 5) ? false : true;
    case 6:
        switch(buf[0]) {
        case 'o':
            return memcmp(buf + 1, "ption", 5) ? false : true;
        case 's':
            return memcmp(buf + 1, "etenv", 5) ? false : true;
        }
        return false;
    case 7:
        return memcmp(buf, "include", 7) ? false : true;
    case 8:
        return memcmp(buf, "errorfmt", 8) ? false : true;
    case 11:
        return memcmp(buf, "load-syntax", 11) ? false : true;
    }
    return false;
}
