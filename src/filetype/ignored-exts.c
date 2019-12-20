static bool is_ignored_extension(const char *s, size_t len)
{
    switch (len) {
    case 3:
        switch (s[0]) {
        case 'b': return mem_equal(s, "bak", len);
        case 'n': return mem_equal(s, "new", len);
        case 'o': return mem_equal(s, "old", len);
        }
        break;
    case 4: return mem_equal(s, "orig", len);
    case 6:
        switch (s[0]) {
        case 'p': return mem_equal(s, "pacnew", len);
        case 'r': return mem_equal(s, "rpmnew", len);
        }
        break;
    case 7:
        switch (s[0]) {
        case 'r': return mem_equal(s, "rpmsave", len);
        case 'p':
            switch (s[3]) {
            case 'o': return mem_equal(s, "pacorig", len);
            case 's': return mem_equal(s, "pacsave", len);
            }
            break;
        }
        break;
    case 8: return mem_equal(s, "dpkg-old", len);
    case 9: return mem_equal(s, "dpkg-dist", len);
    }
    return false;
}
