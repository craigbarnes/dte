static unsigned short lookup_attr(const char *buf, size_t len)
{
    switch(len) {
    case 3:
        return memcmp(buf, "dim", 3) ? 0 : ATTR_DIM;
    case 4:
        switch(buf[0]) {
        case 'b':
            return memcmp(buf + 1, "old", 3) ? 0 : ATTR_BOLD;
        case 'k':
            return memcmp(buf + 1, "eep", 3) ? 0 : ATTR_KEEP;
        }
        return 0;
    case 5:
        return memcmp(buf, "blink", 5) ? 0 : ATTR_BLINK;
    case 6:
        return memcmp(buf, "italic", 6) ? 0 : ATTR_ITALIC;
    case 7:
        return memcmp(buf, "reverse", 7) ? 0 : ATTR_REVERSE;
    case 9:
        switch(buf[0]) {
        case 'i':
            return memcmp(buf + 1, "nvisible", 8) ? 0 : ATTR_INVIS;
        case 'u':
            return memcmp(buf + 1, "nderline", 8) ? 0 : ATTR_UNDERLINE;
        }
        return 0;
    case 12:
        return memcmp(buf, "lowintensity", 12) ? 0 : ATTR_DIM;
    }
    return 0;
}
