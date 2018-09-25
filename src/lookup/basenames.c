static const struct {
    const char *const key;
    const FileTypeEnum val;
} filetype_from_basename_table[74] = {
    {".bash_logout", SHELL},
    {".bash_profile", SHELL},
    {".bashrc", SHELL},
    {".clang-format", YAML},
    {".cshrc", SHELL},
    {".drirc", XML},
    {".editorconfig", INI},
    {".emacs", EMACSLISP},
    {".gemrc", YAML},
    {".gitattributes", CONFIG},
    {".gitconfig", INI},
    {".gitmodules", INI},
    {".gnus", EMACSLISP},
    {".indent.pro", INDENT},
    {".inputrc", INPUTRC},
    {".jshintrc", JSON},
    {".luacheckrc", LUA},
    {".luacov", LUA},
    {".profile", SHELL},
    {".zlogin", SHELL},
    {".zlogout", SHELL},
    {".zprofile", SHELL},
    {".zshenv", SHELL},
    {".zshrc", SHELL},
    {"APKBUILD", SHELL},
    {"BSDmakefile", MAKE},
    {"BUILD.bazel", PYTHON},
    {"CMakeLists.txt", CMAKE},
    {"COMMIT_EDITMSG", GITCOMMIT},
    {"Capfile", RUBY},
    {"Cargo.lock", TOML},
    {"Dockerfile", DOCKER},
    {"Doxyfile", DOXYGEN},
    {"GNUmakefile", MAKE},
    {"Gemfile", RUBY},
    {"Gemfile.lock", RUBY},
    {"Kbuild", MAKE},
    {"Makefile", MAKE},
    {"Makefile.am", MAKE},
    {"Makefile.in", MAKE},
    {"PKGBUILD", SHELL},
    {"Project.ede", EMACSLISP},
    {"Rakefile", RUBY},
    {"Vagrantfile", RUBY},
    {"bash_logout", SHELL},
    {"bash_profile", SHELL},
    {"bashrc", SHELL},
    {"config.ld", LUA},
    {"configure.ac", M4},
    {"cshrc", SHELL},
    {"drirc", XML},
    {"git-rebase-todo", GITREBASE},
    {"gitattributes", CONFIG},
    {"gitconfig", INI},
    {"inputrc", INPUTRC},
    {"krb5.conf", INI},
    {"makefile", MAKE},
    {"meson.build", MESON},
    {"meson_options.txt", MESON},
    {"mimeapps.list", INI},
    {"mkinitcpio.conf", SHELL},
    {"nginx.conf", NGINX},
    {"pacman.conf", INI},
    {"profile", SHELL},
    {"robots.txt", ROBOTSTXT},
    {"rockspec.in", LUA},
    {"terminalrc", INI},
    {"texmf.cnf", TEXMFCNF},
    {"yum.conf", INI},
    {"zlogin", SHELL},
    {"zlogout", SHELL},
    {"zprofile", SHELL},
    {"zshenv", SHELL},
    {"zshrc", SHELL},
};

static FileTypeEnum filetype_from_basename(const char *s, size_t len)
{
    size_t idx;
    const char *key;
    FileTypeEnum val;
    switch (len) {
    case 5:
        switch (s[0]) {
        case '.':
            idx = 12; // .gnus
            goto compare;
        case 'c':
            idx = 49; // cshrc
            goto compare;
        case 'd':
            idx = 50; // drirc
            goto compare;
        case 'z':
            idx = 73; // zshrc
            goto compare;
        }
        break;
    case 6:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'c':
                idx = 4; // .cshrc
                goto compare;
            case 'd':
                idx = 5; // .drirc
                goto compare;
            case 'e':
                idx = 7; // .emacs
                goto compare;
            case 'g':
                idx = 8; // .gemrc
                goto compare;
            case 'z':
                idx = 23; // .zshrc
                goto compare;
            }
            break;
        case 'K':
            idx = 36; // Kbuild
            goto compare;
        case 'b':
            idx = 46; // bashrc
            goto compare;
        case 'z':
            switch (s[1]) {
            case 'l':
                idx = 69; // zlogin
                goto compare;
            case 's':
                idx = 72; // zshenv
                goto compare;
            }
            break;
        }
        break;
    case 7:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'b':
                idx = 2; // .bashrc
                goto compare;
            case 'l':
                idx = 17; // .luacov
                goto compare;
            case 'z':
                switch (s[2]) {
                case 'l':
                    idx = 19; // .zlogin
                    goto compare;
                case 's':
                    idx = 22; // .zshenv
                    goto compare;
                }
                break;
            }
            break;
        case 'C':
            idx = 29; // Capfile
            goto compare;
        case 'G':
            idx = 34; // Gemfile
            goto compare;
        case 'i':
            idx = 54; // inputrc
            goto compare;
        case 'p':
            idx = 63; // profile
            goto compare;
        case 'z':
            idx = 70; // zlogout
            goto compare;
        }
        break;
    case 8:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'i':
                idx = 14; // .inputrc
                goto compare;
            case 'p':
                idx = 18; // .profile
                goto compare;
            case 'z':
                idx = 20; // .zlogout
                goto compare;
            }
            break;
        case 'A':
            idx = 24; // APKBUILD
            goto compare;
        case 'D':
            idx = 32; // Doxyfile
            goto compare;
        case 'M':
            idx = 37; // Makefile
            goto compare;
        case 'P':
            idx = 40; // PKGBUILD
            goto compare;
        case 'R':
            idx = 42; // Rakefile
            goto compare;
        case 'm':
            idx = 56; // makefile
            goto compare;
        case 'y':
            idx = 68; // yum.conf
            goto compare;
        case 'z':
            idx = 71; // zprofile
            goto compare;
        }
        break;
    case 9:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'j':
                idx = 15; // .jshintrc
                goto compare;
            case 'z':
                idx = 21; // .zprofile
                goto compare;
            }
            break;
        case 'c':
            idx = 47; // config.ld
            goto compare;
        case 'g':
            idx = 53; // gitconfig
            goto compare;
        case 'k':
            idx = 55; // krb5.conf
            goto compare;
        case 't':
            idx = 67; // texmf.cnf
            goto compare;
        }
        break;
    case 10:
        switch (s[0]) {
        case '.':
            idx = 10; // .gitconfig
            goto compare;
        case 'C':
            idx = 30; // Cargo.lock
            goto compare;
        case 'D':
            idx = 31; // Dockerfile
            goto compare;
        case 'n':
            idx = 61; // nginx.conf
            goto compare;
        case 'r':
            idx = 64; // robots.txt
            goto compare;
        case 't':
            idx = 66; // terminalrc
            goto compare;
        }
        break;
    case 11:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'g':
                idx = 11; // .gitmodules
                goto compare;
            case 'i':
                idx = 13; // .indent.pro
                goto compare;
            case 'l':
                idx = 16; // .luacheckrc
                goto compare;
            }
            break;
        case 'B':
            switch (s[1]) {
            case 'S':
                idx = 25; // BSDmakefile
                goto compare;
            case 'U':
                idx = 26; // BUILD.bazel
                goto compare;
            }
            break;
        case 'G':
            idx = 33; // GNUmakefile
            goto compare;
        case 'M':
            switch (s[9]) {
            case 'a':
                idx = 38; // Makefile.am
                goto compare;
            case 'i':
                idx = 39; // Makefile.in
                goto compare;
            }
            break;
        case 'P':
            idx = 41; // Project.ede
            goto compare;
        case 'V':
            idx = 43; // Vagrantfile
            goto compare;
        case 'b':
            idx = 44; // bash_logout
            goto compare;
        case 'm':
            idx = 57; // meson.build
            goto compare;
        case 'p':
            idx = 62; // pacman.conf
            goto compare;
        case 'r':
            idx = 65; // rockspec.in
            goto compare;
        }
        break;
    case 12:
        switch (s[0]) {
        case '.':
            idx = 0; // .bash_logout
            goto compare;
        case 'G':
            idx = 35; // Gemfile.lock
            goto compare;
        case 'b':
            idx = 45; // bash_profile
            goto compare;
        case 'c':
            idx = 48; // configure.ac
            goto compare;
        }
        break;
    case 13:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'b':
                idx = 1; // .bash_profile
                goto compare;
            case 'c':
                idx = 3; // .clang-format
                goto compare;
            case 'e':
                idx = 6; // .editorconfig
                goto compare;
            }
            break;
        case 'g':
            idx = 52; // gitattributes
            goto compare;
        case 'm':
            idx = 59; // mimeapps.list
            goto compare;
        }
        break;
    case 14:
        switch (s[0]) {
        case '.':
            idx = 9; // .gitattributes
            goto compare;
        case 'C':
            switch (s[1]) {
            case 'M':
                idx = 27; // CMakeLists.txt
                goto compare;
            case 'O':
                idx = 28; // COMMIT_EDITMSG
                goto compare;
            }
            break;
        }
        break;
    case 15:
        switch (s[0]) {
        case 'g':
            idx = 51; // git-rebase-todo
            goto compare;
        case 'm':
            idx = 60; // mkinitcpio.conf
            goto compare;
        }
        break;
    case 17:
        idx = 58; // meson_options.txt
        goto compare;
    }
    return 0;
compare:
    key = filetype_from_basename_table[idx].key;
    val = filetype_from_basename_table[idx].val;
    return memcmp(s, key, len) ? 0 : val;
}
