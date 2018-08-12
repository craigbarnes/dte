static FileTypeEnum filetype_from_interpreter(const char *s, size_t len)
{
    switch (len) {
    case 2:
        return memcmp(s, "sh", 2) ? 0 : SHELL;
    case 3:
        switch (s[0]) {
        case 'a':
            switch (s[1]) {
            case 's':
                return (s[2] != 'h') ? 0 : SHELL;
            case 'w':
                return (s[2] != 'k') ? 0 : AWK;
            }
            return 0;
        case 'c':
            return memcmp(s + 1, "cl", 2) ? 0 : COMMONLISP;
        case 'e':
            return memcmp(s + 1, "cl", 2) ? 0 : COMMONLISP;
        case 'k':
            return memcmp(s + 1, "sh", 2) ? 0 : SHELL;
        case 'l':
            return memcmp(s + 1, "ua", 2) ? 0 : LUA;
        case 'p':
            return memcmp(s + 1, "hp", 2) ? 0 : PHP;
        case 's':
            return memcmp(s + 1, "ed", 2) ? 0 : SED;
        case 't':
            return memcmp(s + 1, "cc", 2) ? 0 : C;
        case 'z':
            return memcmp(s + 1, "sh", 2) ? 0 : SHELL;
        }
        return 0;
    case 4:
        switch (s[0]) {
        case 'b':
            return memcmp(s + 1, "ash", 3) ? 0 : SHELL;
        case 'd':
            return memcmp(s + 1, "ash", 3) ? 0 : SHELL;
        case 'g':
            switch (s[1]) {
            case 'a':
                return memcmp(s + 2, "wk", 2) ? 0 : AWK;
            case 's':
                return memcmp(s + 2, "ed", 2) ? 0 : SED;
            }
            return 0;
        case 'l':
            return memcmp(s + 1, "isp", 3) ? 0 : COMMONLISP;
        case 'm':
            switch (s[1]) {
            case 'a':
                switch (s[2]) {
                case 'k':
                    return (s[3] != 'e') ? 0 : MAKE;
                case 'w':
                    return (s[3] != 'k') ? 0 : AWK;
                }
                return 0;
            case 'k':
                return memcmp(s + 2, "sh", 2) ? 0 : SHELL;
            case 'o':
                return memcmp(s + 2, "on", 2) ? 0 : MOONSCRIPT;
            }
            return 0;
        case 'n':
            switch (s[1]) {
            case 'a':
                return memcmp(s + 2, "wk", 2) ? 0 : AWK;
            case 'o':
                return memcmp(s + 2, "de", 2) ? 0 : JAVASCRIPT;
            }
            return 0;
        case 'p':
            return memcmp(s + 1, "erl", 3) ? 0 : PERL;
        case 'r':
            switch (s[1]) {
            case '6':
                return memcmp(s + 2, "rs", 2) ? 0 : SCHEME;
            case 'a':
                return memcmp(s + 2, "ke", 2) ? 0 : RUBY;
            case 'u':
                return memcmp(s + 2, "by", 2) ? 0 : RUBY;
            }
            return 0;
        case 's':
            return memcmp(s + 1, "bcl", 3) ? 0 : COMMONLISP;
        case 'w':
            return memcmp(s + 1, "ish", 3) ? 0 : TCL;
        }
        return 0;
    case 5:
        switch (s[0]) {
        case 'c':
            return memcmp(s + 1, "lisp", 4) ? 0 : COMMONLISP;
        case 'g':
            switch (s[1]) {
            case 'm':
                return memcmp(s + 2, "ake", 3) ? 0 : MAKE;
            case 'u':
                return memcmp(s + 2, "ile", 3) ? 0 : SCHEME;
            }
            return 0;
        case 'j':
            return memcmp(s + 1, "ruby", 4) ? 0 : RUBY;
        case 'p':
            return memcmp(s + 1, "dksh", 4) ? 0 : SHELL;
        case 't':
            return memcmp(s + 1, "clsh", 4) ? 0 : TCL;
        }
        return 0;
    case 6:
        switch (s[0]) {
        case 'b':
            return memcmp(s + 1, "igloo", 5) ? 0 : SCHEME;
        case 'c':
            return memcmp(s + 1, "offee", 5) ? 0 : COFFEESCRIPT;
        case 'g':
            return memcmp(s + 1, "roovy", 5) ? 0 : GROOVY;
        case 'l':
            return memcmp(s + 1, "uajit", 5) ? 0 : LUA;
        case 'p':
            return memcmp(s + 1, "ython", 5) ? 0 : PYTHON;
        case 'r':
            return memcmp(s + 1, "acket", 5) ? 0 : SCHEME;
        }
        return 0;
    case 7:
        switch (s[0]) {
        case 'c':
            switch (s[1]) {
            case 'h':
                return memcmp(s + 2, "icken", 5) ? 0 : SCHEME;
            case 'r':
                return memcmp(s + 2, "ystal", 5) ? 0 : RUBY;
            }
            return 0;
        case 'g':
            return memcmp(s + 1, "nuplot", 6) ? 0 : GNUPLOT;
        case 'm':
            return memcmp(s + 1, "acruby", 6) ? 0 : RUBY;
        }
        return 0;
    case 10:
        switch (s[0]) {
        case 'o':
            return memcmp(s + 1, "penrc-run", 9) ? 0 : SHELL;
        case 'r':
            return memcmp(s + 1, "unhaskell", 9) ? 0 : HASKELL;
        }
        return 0;
    }
    return 0;
}
