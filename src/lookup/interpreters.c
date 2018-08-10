static FileTypeEnum filetype_from_interpreter(const char *buf, size_t len)
{
    switch(len) {
    case 2:
        return memcmp(buf, "sh", 2) ? 0 : SHELL;
    case 3:
        switch(buf[0]) {
        case 'a':
            switch(buf[1]) {
            case 's':
                return (buf[2] != 'h') ? 0 : SHELL;
            case 'w':
                return (buf[2] != 'k') ? 0 : AWK;
            }
            return 0;
        case 'c':
            return memcmp(buf + 1, "cl", 2) ? 0 : COMMONLISP;
        case 'e':
            return memcmp(buf + 1, "cl", 2) ? 0 : COMMONLISP;
        case 'k':
            return memcmp(buf + 1, "sh", 2) ? 0 : SHELL;
        case 'l':
            return memcmp(buf + 1, "ua", 2) ? 0 : LUA;
        case 'p':
            return memcmp(buf + 1, "hp", 2) ? 0 : PHP;
        case 's':
            return memcmp(buf + 1, "ed", 2) ? 0 : SED;
        case 't':
            return memcmp(buf + 1, "cc", 2) ? 0 : C;
        case 'z':
            return memcmp(buf + 1, "sh", 2) ? 0 : SHELL;
        }
        return 0;
    case 4:
        switch(buf[0]) {
        case 'b':
            return memcmp(buf + 1, "ash", 3) ? 0 : SHELL;
        case 'd':
            return memcmp(buf + 1, "ash", 3) ? 0 : SHELL;
        case 'g':
            switch(buf[1]) {
            case 'a':
                return memcmp(buf + 2, "wk", 2) ? 0 : AWK;
            case 's':
                return memcmp(buf + 2, "ed", 2) ? 0 : SED;
            }
            return 0;
        case 'l':
            return memcmp(buf + 1, "isp", 3) ? 0 : COMMONLISP;
        case 'm':
            switch(buf[1]) {
            case 'a':
                switch(buf[2]) {
                case 'k':
                    return (buf[3] != 'e') ? 0 : MAKE;
                case 'w':
                    return (buf[3] != 'k') ? 0 : AWK;
                }
                return 0;
            case 'k':
                return memcmp(buf + 2, "sh", 2) ? 0 : SHELL;
            case 'o':
                return memcmp(buf + 2, "on", 2) ? 0 : MOONSCRIPT;
            }
            return 0;
        case 'n':
            switch(buf[1]) {
            case 'a':
                return memcmp(buf + 2, "wk", 2) ? 0 : AWK;
            case 'o':
                return memcmp(buf + 2, "de", 2) ? 0 : JAVASCRIPT;
            }
            return 0;
        case 'p':
            return memcmp(buf + 1, "erl", 3) ? 0 : PERL;
        case 'r':
            switch(buf[1]) {
            case '6':
                return memcmp(buf + 2, "rs", 2) ? 0 : SCHEME;
            case 'a':
                return memcmp(buf + 2, "ke", 2) ? 0 : RUBY;
            case 'u':
                return memcmp(buf + 2, "by", 2) ? 0 : RUBY;
            }
            return 0;
        case 's':
            return memcmp(buf + 1, "bcl", 3) ? 0 : COMMONLISP;
        case 'w':
            return memcmp(buf + 1, "ish", 3) ? 0 : TCL;
        }
        return 0;
    case 5:
        switch(buf[0]) {
        case 'c':
            return memcmp(buf + 1, "lisp", 4) ? 0 : COMMONLISP;
        case 'g':
            switch(buf[1]) {
            case 'm':
                return memcmp(buf + 2, "ake", 3) ? 0 : MAKE;
            case 'u':
                return memcmp(buf + 2, "ile", 3) ? 0 : SCHEME;
            }
            return 0;
        case 'j':
            return memcmp(buf + 1, "ruby", 4) ? 0 : RUBY;
        case 'p':
            return memcmp(buf + 1, "dksh", 4) ? 0 : SHELL;
        case 't':
            return memcmp(buf + 1, "clsh", 4) ? 0 : TCL;
        }
        return 0;
    case 6:
        switch(buf[0]) {
        case 'b':
            return memcmp(buf + 1, "igloo", 5) ? 0 : SCHEME;
        case 'c':
            return memcmp(buf + 1, "offee", 5) ? 0 : COFFEESCRIPT;
        case 'g':
            return memcmp(buf + 1, "roovy", 5) ? 0 : GROOVY;
        case 'l':
            return memcmp(buf + 1, "uajit", 5) ? 0 : LUA;
        case 'p':
            return memcmp(buf + 1, "ython", 5) ? 0 : PYTHON;
        case 'r':
            return memcmp(buf + 1, "acket", 5) ? 0 : SCHEME;
        }
        return 0;
    case 7:
        switch(buf[0]) {
        case 'c':
            switch(buf[1]) {
            case 'h':
                return memcmp(buf + 2, "icken", 5) ? 0 : SCHEME;
            case 'r':
                return memcmp(buf + 2, "ystal", 5) ? 0 : RUBY;
            }
            return 0;
        case 'g':
            return memcmp(buf + 1, "nuplot", 6) ? 0 : GNUPLOT;
        case 'm':
            return memcmp(buf + 1, "acruby", 6) ? 0 : RUBY;
        }
        return 0;
    case 10:
        switch(buf[0]) {
        case 'o':
            return memcmp(buf + 1, "penrc-run", 9) ? 0 : SHELL;
        case 'r':
            return memcmp(buf + 1, "unhaskell", 9) ? 0 : HASKELL;
        }
        return 0;
    }
    return 0;
}
