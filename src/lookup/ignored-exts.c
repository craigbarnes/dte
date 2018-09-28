static bool is_ignored_extension(const char *s, size_t len)
{
    switch (len) {
    case 3:
        switch (s[0]) {
        case 'b': return !memcmp(s, "bak", len);
        case 'n': return !memcmp(s, "new", len);
        case 'o': return !memcmp(s, "old", len);
        }
        break;
    case 4: return !memcmp(s, "orig", len);
    case 6:
        switch (s[0]) {
        case 'p': return !memcmp(s, "pacnew", len);
        case 'r': return !memcmp(s, "rpmnew", len);
        }
        break;
    case 7:
        switch (s[0]) {
        case 'r': return !memcmp(s, "rpmsave", len);
        case 'p':
            switch (s[3]) {
            case 'o': return !memcmp(s, "pacorig", len);
            case 's': return !memcmp(s, "pacsave", len);
            }
            break;
        }
        break;
    case 8: return !memcmp(s, "dpkg-old", len);
    case 9: return !memcmp(s, "dpkg-dist", len);
    }
    return false;
}
