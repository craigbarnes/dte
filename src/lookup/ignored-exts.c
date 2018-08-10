static bool is_ignored_extension(const char *s, size_t len)
{
    switch(len) {
    case 3:
        switch(s[0]) {
        case 'b':
            return memcmp(s + 1, "ak", 2) ? false : true;
        case 'n':
            return memcmp(s + 1, "ew", 2) ? false : true;
        case 'o':
            return memcmp(s + 1, "ld", 2) ? false : true;
        }
        return false;
    case 4:
        return memcmp(s, "orig", 4) ? false : true;
    case 6:
        switch(s[0]) {
        case 'p':
            return memcmp(s + 1, "acnew", 5) ? false : true;
        case 'r':
            return memcmp(s + 1, "pmnew", 5) ? false : true;
        }
        return false;
    case 7:
        switch(s[0]) {
        case 'p':
            if (memcmp(s + 1, "ac", 2)) {
                return false;
            }
            switch(s[3]) {
            case 'o':
                return memcmp(s + 4, "rig", 3) ? false : true;
            case 's':
                return memcmp(s + 4, "ave", 3) ? false : true;
            }
            return false;
        case 'r':
            return memcmp(s + 1, "pmsave", 6) ? false : true;
        }
        return false;
    case 8:
        return memcmp(s, "dpkg-old", 8) ? false : true;
    case 9:
        return memcmp(s, "dpkg-dist", 9) ? false : true;
    }
    return false;
}
