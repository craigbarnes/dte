static short lookup_color(const char *s, size_t len)
{
    switch (len) {
    case 3:
        return memcmp(s, "red", 3) ? -3 : 1;
    case 4:
        switch (s[0]) {
        case 'b':
            return memcmp(s + 1, "lue", 3) ? -3 : 4;
        case 'c':
            return memcmp(s + 1, "yan", 3) ? -3 : 6;
        case 'g':
            return memcmp(s + 1, "ray", 3) ? -3 : 7;
        case 'k':
            return memcmp(s + 1, "eep", 3) ? -3 : -2;
        }
        return -3;
    case 5:
        switch (s[0]) {
        case 'b':
            return memcmp(s + 1, "lack", 4) ? -3 : 0;
        case 'g':
            return memcmp(s + 1, "reen", 4) ? -3 : 2;
        case 'w':
            return memcmp(s + 1, "hite", 4) ? -3 : 15;
        }
        return -3;
    case 6:
        return memcmp(s, "yellow", 6) ? -3 : 3;
    case 7:
        switch (s[0]) {
        case 'd':
            return memcmp(s + 1, "efault", 6) ? -3 : -1;
        case 'm':
            return memcmp(s + 1, "agenta", 6) ? -3 : 5;
        }
        return -3;
    case 8:
        switch (s[0]) {
        case 'd':
            return memcmp(s + 1, "arkgray", 7) ? -3 : 8;
        case 'l':
            return memcmp(s + 1, "ightred", 7) ? -3 : 9;
        }
        return -3;
    case 9:
        if (memcmp(s, "light", 5)) {
            return -3;
        }
        switch (s[5]) {
        case 'b':
            return memcmp(s + 6, "lue", 3) ? -3 : 12;
        case 'c':
            return memcmp(s + 6, "yan", 3) ? -3 : 14;
        }
        return -3;
    case 10:
        return memcmp(s, "lightgreen", 10) ? -3 : 10;
    case 11:
        return memcmp(s, "lightyellow", 11) ? -3 : 11;
    case 12:
        return memcmp(s, "lightmagenta", 12) ? -3 : 13;
    }
    return -3;
}
