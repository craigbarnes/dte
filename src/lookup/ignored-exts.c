static bool is_ignored_extension(const char *buf, size_t len)
{
    switch(len) {
    case 3:
        switch(buf[0]) {
        case 'b':
            return memcmp(buf + 1, "ak", 2) ? false : true;
        case 'n':
            return memcmp(buf + 1, "ew", 2) ? false : true;
        case 'o':
            return memcmp(buf + 1, "ld", 2) ? false : true;
        }
        return false;
    case 4:
        return memcmp(buf, "orig", 4) ? false : true;
    case 6:
        switch(buf[0]) {
        case 'p':
            return memcmp(buf + 1, "acnew", 5) ? false : true;
        case 'r':
            return memcmp(buf + 1, "pmnew", 5) ? false : true;
        }
        return false;
    case 7:
        switch(buf[0]) {
        case 'p':
            if (memcmp(buf + 1, "ac", 2)) {
                return false;
            }
            switch(buf[3]) {
            case 'o':
                return memcmp(buf + 4, "rig", 3) ? false : true;
            case 's':
                return memcmp(buf + 4, "ave", 3) ? false : true;
            }
            return false;
        case 'r':
            return memcmp(buf + 1, "pmsave", 6) ? false : true;
        }
        return false;
    case 8:
        return memcmp(buf, "dpkg-old", 8) ? false : true;
    case 9:
        return memcmp(buf, "dpkg-dist", 9) ? false : true;
    }
    return false;
}
