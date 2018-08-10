static FileTypeEnum filetype_from_basename(const char *buf, size_t len)
{
    switch(len) {
    case 5:
        switch(buf[0]) {
        case 'c':
            return memcmp(buf + 1, "shrc", 4) ? 0 : SHELL;
        case 'd':
            return memcmp(buf + 1, "rirc", 4) ? 0 : XML;
        case 'z':
            return memcmp(buf + 1, "shrc", 4) ? 0 : SHELL;
        }
        return 0;
    case 6:
        switch(buf[0]) {
        case 'K':
            return memcmp(buf + 1, "build", 5) ? 0 : MAKE;
        case 'b':
            return memcmp(buf + 1, "ashrc", 5) ? 0 : SHELL;
        case 'z':
            switch(buf[1]) {
            case 'l':
                return memcmp(buf + 2, "ogin", 4) ? 0 : SHELL;
            case 's':
                return memcmp(buf + 2, "henv", 4) ? 0 : SHELL;
            }
            return 0;
        }
        return 0;
    case 7:
        switch(buf[0]) {
        case 'C':
            return memcmp(buf + 1, "apfile", 6) ? 0 : RUBY;
        case 'G':
            return memcmp(buf + 1, "emfile", 6) ? 0 : RUBY;
        case 'i':
            return memcmp(buf + 1, "nputrc", 6) ? 0 : INPUTRC;
        case 'p':
            return memcmp(buf + 1, "rofile", 6) ? 0 : SHELL;
        case 'z':
            return memcmp(buf + 1, "logout", 6) ? 0 : SHELL;
        }
        return 0;
    case 8:
        switch(buf[0]) {
        case 'A':
            return memcmp(buf + 1, "PKBUILD", 7) ? 0 : SHELL;
        case 'D':
            return memcmp(buf + 1, "oxyfile", 7) ? 0 : DOXYGEN;
        case 'M':
            return memcmp(buf + 1, "akefile", 7) ? 0 : MAKE;
        case 'P':
            return memcmp(buf + 1, "KGBUILD", 7) ? 0 : SHELL;
        case 'R':
            return memcmp(buf + 1, "akefile", 7) ? 0 : RUBY;
        case 'm':
            return memcmp(buf + 1, "akefile", 7) ? 0 : MAKE;
        case 'y':
            return memcmp(buf + 1, "um.conf", 7) ? 0 : INI;
        case 'z':
            return memcmp(buf + 1, "profile", 7) ? 0 : SHELL;
        }
        return 0;
    case 9:
        switch(buf[0]) {
        case 'c':
            return memcmp(buf + 1, "onfig.ld", 8) ? 0 : LUA;
        case 'g':
            return memcmp(buf + 1, "itconfig", 8) ? 0 : INI;
        case 'k':
            return memcmp(buf + 1, "rb5.conf", 8) ? 0 : INI;
        case 't':
            return memcmp(buf + 1, "exmf.cnf", 8) ? 0 : TEXMFCNF;
        }
        return 0;
    case 10:
        switch(buf[0]) {
        case 'C':
            return memcmp(buf + 1, "argo.lock", 9) ? 0 : TOML;
        case 'D':
            return memcmp(buf + 1, "ockerfile", 9) ? 0 : DOCKER;
        case 'n':
            return memcmp(buf + 1, "ginx.conf", 9) ? 0 : NGINX;
        case 'r':
            return memcmp(buf + 1, "obots.txt", 9) ? 0 : ROBOTSTXT;
        case 't':
            return memcmp(buf + 1, "erminalrc", 9) ? 0 : INI;
        }
        return 0;
    case 11:
        switch(buf[0]) {
        case 'B':
            switch(buf[1]) {
            case 'S':
                return memcmp(buf + 2, "Dmakefile", 9) ? 0 : MAKE;
            case 'U':
                return memcmp(buf + 2, "ILD.bazel", 9) ? 0 : PYTHON;
            }
            return 0;
        case 'G':
            return memcmp(buf + 1, "NUmakefile", 10) ? 0 : MAKE;
        case 'M':
            if (memcmp(buf + 1, "akefile.", 8)) {
                return 0;
            }
            switch(buf[9]) {
            case 'a':
                return (buf[10] != 'm') ? 0 : MAKE;
            case 'i':
                return (buf[10] != 'n') ? 0 : MAKE;
            }
            return 0;
        case 'P':
            return memcmp(buf + 1, "roject.ede", 10) ? 0 : EMACSLISP;
        case 'V':
            return memcmp(buf + 1, "agrantfile", 10) ? 0 : RUBY;
        case 'b':
            return memcmp(buf + 1, "ash_logout", 10) ? 0 : SHELL;
        case 'm':
            return memcmp(buf + 1, "eson.build", 10) ? 0 : MESON;
        case 'p':
            return memcmp(buf + 1, "acman.conf", 10) ? 0 : INI;
        case 'r':
            return memcmp(buf + 1, "ockspec.in", 10) ? 0 : LUA;
        }
        return 0;
    case 12:
        switch(buf[0]) {
        case 'G':
            return memcmp(buf + 1, "emfile.lock", 11) ? 0 : RUBY;
        case 'b':
            return memcmp(buf + 1, "ash_profile", 11) ? 0 : SHELL;
        }
        return 0;
    case 13:
        switch(buf[0]) {
        case 'g':
            return memcmp(buf + 1, "itattributes", 12) ? 0 : CONFIG;
        case 'm':
            return memcmp(buf + 1, "imeapps.list", 12) ? 0 : INI;
        }
        return 0;
    case 14:
        switch(buf[0]) {
        case 'C':
            switch(buf[1]) {
            case 'M':
                return memcmp(buf + 2, "akeLists.txt", 12) ? 0 : CMAKE;
            case 'O':
                return memcmp(buf + 2, "MMIT_EDITMSG", 12) ? 0 : GITCOMMIT;
            }
            return 0;
        }
        return 0;
    case 15:
        switch(buf[0]) {
        case 'g':
            return memcmp(buf + 1, "it-rebase-todo", 14) ? 0 : GITREBASE;
        case 'm':
            return memcmp(buf + 1, "kinitcpio.conf", 14) ? 0 : SHELL;
        }
        return 0;
    case 17:
        return memcmp(buf, "meson_options.txt", 17) ? 0 : MESON;
    }
    return 0;
}
