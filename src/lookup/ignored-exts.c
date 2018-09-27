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

#define CMP(i) idx = i; goto compare
#define CMPN(i) idx = i; goto compare_last_char
#define KEY is_ignored_extension_table[idx].key
#define VAL is_ignored_extension_table[idx].val

static bool is_ignored_extension(const char *s, size_t len)
{
    size_t idx;
    switch (len) {
    case 3:
        switch (s[0]) {
        case 'b': CMP(0); // bak
        case 'n': CMP(3); // new
        case 'o': CMP(4); // old
        }
        break;
    case 4: CMP(5); // orig
    case 6:
        switch (s[0]) {
        case 'p': CMP(6); // pacnew
        case 'r': CMP(9); // rpmnew
        }
        break;
    case 7:
        switch (s[0]) {
        case 'p':
            switch (s[3]) {
            case 'o': CMP(7); // pacorig
            case 's': CMP(8); // pacsave
            }
            break;
        case 'r': CMP(10); // rpmsave
        }
        break;
    case 8: CMP(2); // dpkg-old
    case 9: CMP(1); // dpkg-dist
    }
    return false;
compare:
    return (memcmp(s, KEY, len) == 0) ? VAL : false;
compare_last_char:
    return (s[len - 1] == KEY[len - 1]) ? VAL : false;
}

#undef CMP
#undef CMPN
#undef KEY
#undef VAL
