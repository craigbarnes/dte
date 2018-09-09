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
        return memcmp(s, "darkgray", 8) ? -3 : 8;
    }
    return -3;
}
