static unsigned short lookup_attr(const char *s, size_t len)
{
    switch(len) {
    case 3:
        return memcmp(s, "dim", 3) ? 0 : ATTR_DIM;
    case 4:
        switch(s[0]) {
        case 'b':
            return memcmp(s + 1, "old", 3) ? 0 : ATTR_BOLD;
        case 'k':
            return memcmp(s + 1, "eep", 3) ? 0 : ATTR_KEEP;
        }
        return 0;
    case 5:
        return memcmp(s, "blink", 5) ? 0 : ATTR_BLINK;
    case 6:
        return memcmp(s, "italic", 6) ? 0 : ATTR_ITALIC;
    case 7:
        return memcmp(s, "reverse", 7) ? 0 : ATTR_REVERSE;
    case 9:
        switch(s[0]) {
        case 'i':
            return memcmp(s + 1, "nvisible", 8) ? 0 : ATTR_INVIS;
        case 'u':
            return memcmp(s + 1, "nderline", 8) ? 0 : ATTR_UNDERLINE;
        }
        return 0;
    case 12:
        return memcmp(s, "lowintensity", 12) ? 0 : ATTR_DIM;
    }
    return 0;
}
