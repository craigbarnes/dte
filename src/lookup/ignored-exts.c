static const struct {
    const char key[16];
    const bool val;
} is_ignored_extension_table[11] = {
    {"bak", true},
    {"dpkg-dist", true},
    {"dpkg-old", true},
    {"new", true},
    {"old", true},
    {"orig", true},
    {"pacnew", true},
    {"pacorig", true},
    {"pacsave", true},
    {"rpmnew", true},
    {"rpmsave", true},
};

static bool is_ignored_extension(const char *s, size_t len)
{
    size_t idx;
    const char *key;
    bool val;
    switch (len) {
    case 3:
        switch (s[0]) {
        case 'b':
            idx = 0; // bak
            goto compare;
        case 'n':
            idx = 3; // new
            goto compare;
        case 'o':
            idx = 4; // old
            goto compare;
        }
        break;
    case 4:
        idx = 5; // orig
        goto compare;
    case 6:
        switch (s[0]) {
        case 'p':
            idx = 6; // pacnew
            goto compare;
        case 'r':
            idx = 9; // rpmnew
            goto compare;
        }
        break;
    case 7:
        switch (s[0]) {
        case 'p':
            switch (s[3]) {
            case 'o':
                idx = 7; // pacorig
                goto compare;
            case 's':
                idx = 8; // pacsave
                goto compare;
            }
            break;
        case 'r':
            idx = 10; // rpmsave
            goto compare;
        }
        break;
    case 8:
        idx = 2; // dpkg-old
        goto compare;
    case 9:
        idx = 1; // dpkg-dist
        goto compare;
    }
    return false;
compare:
    key = is_ignored_extension_table[idx].key;
    val = is_ignored_extension_table[idx].val;
    return memcmp(s, key, len) ? false : val;
}
